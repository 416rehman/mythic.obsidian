// Copyright Stellar Games. All Rights Reserved.

#include "Stats/MythicStatDefinition.h"

#include "Internationalization/Text.h"
#include "System/MythicAssetManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace {
bool HasStableStatStringTableIdentity(const FText& Text) {
    FName TableId;
    FString Key;
    return !Text.IsEmpty() && FTextInspector::GetTableIdAndKey(Text, TableId, Key)
        && !TableId.IsNone() && !Key.IsEmpty();
}

template <typename EnumType>
bool IsValidEnumValue(EnumType Value) {
    const UEnum* Enum = StaticEnum<EnumType>();
    return Enum && Enum->IsValidEnumValue(static_cast<int64>(Value));
}
}

FPrimaryAssetId UMythicStatDefinition::GetPrimaryAssetId() const {
    return FPrimaryAssetId(UMythicAssetManager::StatDefinitionType, StatTag.GetTagName());
}

bool UMythicStatDefinition::AppendValidationErrors(TArray<FText>& OutErrors) const {
    const int32 InitialErrorCount = OutErrors.Num();

    if (DeveloperName.IsNone()) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "MissingDeveloperName", "DeveloperName is required."));
    }
    if (DesignerPurpose.TrimStartAndEnd().IsEmpty()) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "MissingDesignerPurpose", "DesignerPurpose is required."));
    }
    if (Revision < 1) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidRevision", "Revision must be at least 1."));
    }
    if (PresentationRevision < 1) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidPresentationRevision", "PresentationRevision must be at least 1."));
    }
    if (!StatTag.IsValid() || !StatTag.ToString().StartsWith(TEXT("Stat.Attribute."))) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidStatTag", "StatTag must be a valid child of Stat.Attribute."));
    }
    if (!Attribute.IsValid()) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidAttribute", "Attribute must resolve to a real GAS attribute property."));
    }
    if (!HasStableStatStringTableIdentity(DisplayName)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidDisplayName", "DisplayName must be nonempty and use a stable String Table identity."));
    }
    if (!Description.IsEmpty() && !HasStableStatStringTableIdentity(Description)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidDescription", "A nonempty Description must use a stable String Table identity."));
    }
    if (Category.Asset.IsNull() || !Category.IsValid()) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidCategory", "Category must directly reference a valid Stat Category Definition asset."));
    }
    if (NumberPresentation.DecimalPlaces < 0 || NumberPresentation.DecimalPlaces > 4) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidDecimalPlaces", "DecimalPlaces must be between 0 and 4."));
    }
    if (!IsValidEnumValue(NumberPresentation.Format)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidFormat", "NumberPresentation.Format is invalid."));
    }
    if (!NumberPresentation.UnitSuffix.IsEmpty() && !HasStableStatStringTableIdentity(NumberPresentation.UnitSuffix)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidUnitSuffix", "A nonempty UnitSuffix must use a stable String Table identity."));
    }
    if (!IsValidEnumValue(ComparisonDirection)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidComparisonDirection", "ComparisonDirection is invalid."));
    }
    if (!FMath::IsFinite(NeutralValue)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidNeutral", "NeutralValue must be finite."));
    }
    if (!IsValidEnumValue(SheetVisibility)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidVisibility", "SheetVisibility is invalid."));
    }
    if (!IsValidEnumValue(PairRole)) {
        OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "InvalidPairRole", "PairRole is invalid."));
    }
    else if (PairRole == EMythicStatPairRole::None) {
        if (PairedStat.IsValid()) {
            OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "UnexpectedPair", "PairedStat must be empty when PairRole is None."));
        }
    }
    else {
        if (PairedStat.Asset.IsNull() || !PairedStat.IsValid()) {
            OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "MissingPair", "Current and Capacity stats require a direct Paired Stat Definition asset reference."));
        }
        else if (PairedStat.GetPrimaryAssetId() == GetPrimaryAssetId()) {
            OutErrors.Add(NSLOCTEXT("MythicStatDefinition", "SelfPair", "A stat cannot pair with itself."));
        }
    }

    return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UMythicStatDefinition::IsDataValid(FDataValidationContext& Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    AppendValidationErrors(Errors);
    for (const FText& Error : Errors) {
        Context.AddError(Error);
    }
    return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty()
        ? EDataValidationResult::Invalid
        : EDataValidationResult::Valid;
}
#endif
