#include "Itemization/Affixes/MythicAffixProfile.h"

#include "Misc/DataValidation.h"
#include "System/MythicAssetManager.h"

#define LOCTEXT_NAMESPACE "MythicAffixProfile"

FPrimaryAssetId UMythicAffixProfile::GetPrimaryAssetId() const {
    return ProfileTag.IsValid() ? FPrimaryAssetId(UMythicAssetManager::AffixProfileType, ProfileTag.GetTagName())
                                : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicAffixProfile::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };
    if (!ProfileTag.IsValid() || !ProfileTag.ToString().StartsWith(TEXT("Itemization.AffixProfile."))) {
        Error(LOCTEXT("InvalidProfileTag", "ProfileTag must be a valid Itemization.AffixProfile.* tag."));
    }
    if (RollPolicy.Asset.IsNull() || !RollPolicy.IsValid() || DeveloperName.IsNone()
        || DesignerPurpose.TrimStartAndEnd().IsEmpty() || Revision < 1) {
        Error(LOCTEXT("InvalidMetadata", "Profile metadata, a direct Roll Policy asset reference, and revision are required."));
    }
    TSet<FGuid> GrantGuids;
    TSet<FGuid> SliceGuids;
    TSet<FPrimaryAssetId> PoolIds;
    auto CheckGrant = [&Error, &GrantGuids](const FMythicAffixGrantSpec &Grant) {
        if (!Grant.GrantGuid.IsValid() || GrantGuids.Contains(Grant.GrantGuid) || Grant.DeveloperName.IsNone()
            || Grant.AffixDefinition.Asset.IsNull() || !Grant.AffixDefinition.IsValid()
            || !Grant.RollGroup.IsValid() || !Grant.SourceKind.IsValid()
            || !MythicAffixGrant::IsTierSelectionValid(Grant.TierMode, Grant.ExactTierRank)) {
            Error(LOCTEXT("InvalidGrant", "Guaranteed grants require unique tool-owned GUIDs, a direct Affix Definition asset reference, a canonical tier selection (weighted or one-based exact rank), a Roll Group, and source."));
        }
        GrantGuids.Add(Grant.GrantGuid);
    };
    for (const FMythicAffixGrantSpec &Grant : GuaranteedGrants) {
        CheckGrant(Grant);
    }
    for (const FMythicAffixPoolSlice &Slice : RandomPoolSlices) {
        if (!Slice.SliceGuid.IsValid() || SliceGuids.Contains(Slice.SliceGuid) || Slice.DeveloperName.IsNone()
            || Slice.Pool.Asset.IsNull() || !Slice.Pool.IsValid()
            || PoolIds.Contains(Slice.Pool.GetPrimaryAssetId()) || !Slice.SourceKind.IsValid()
            || Slice.SliceWeight <= 0) {
            Error(LOCTEXT("InvalidSlice", "Slices require unique GUIDs/pools, developer identity, source, and positive integer weight."));
        }
        SliceGuids.Add(Slice.SliceGuid);
        PoolIds.Add(Slice.Pool.GetPrimaryAssetId());
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
