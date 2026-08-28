#include "World/Harvesting/MythicHarvestableDefinition.h"

#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "System/MythicAssetManager.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "MythicHarvestableDefinition"

namespace {
    template <typename EnumType>
    bool IsValidEnumValue(const EnumType Value) {
        const UEnum *Enum = StaticEnum<EnumType>();
        return Enum && Enum->IsValidEnumValue(static_cast<int64>(Value));
    }

    void AppendRewardErrors(const TArray<FMythicHarvestRewardEntry> &Entries, const TCHAR *ChannelName, TSet<const UItemDefinition *> &SeenItems,
                            TArray<FText> &OutErrors) {
        for (int32 Index = 0; Index < Entries.Num(); ++Index) {
            const FMythicHarvestRewardEntry &Entry = Entries[Index];
            if (!Entry.ItemDefinition) {
                OutErrors.Add(FText::Format(LOCTEXT("MissingRewardItem", "{0} reward row {1} requires a direct Item Definition."),
                                            FText::FromString(ChannelName), FText::AsNumber(Index)));
            }
            else if (SeenItems.Contains(Entry.ItemDefinition)) {
                OutErrors.Add(FText::Format(LOCTEXT("DuplicateRewardItem", "Item Definition '{0}' appears more than once across harvest reward channels."),
                                            FText::FromString(Entry.ItemDefinition->GetPathName())));
            }
            else {
                SeenItems.Add(Entry.ItemDefinition);
            }

            if (Entry.MinQuantity < 1 || Entry.MaxQuantity < Entry.MinQuantity) {
                OutErrors.Add(FText::Format(LOCTEXT("InvalidRewardQuantity", "{0} reward row {1} requires an inclusive positive quantity range."),
                                            FText::FromString(ChannelName), FText::AsNumber(Index)));
            }
            if (!FMath::IsFinite(Entry.Probability) || Entry.Probability < 0.0f || Entry.Probability > 1.0f) {
                OutErrors.Add(FText::Format(LOCTEXT("InvalidRewardProbability", "{0} reward row {1} probability must be finite and in [0,1]."),
                                            FText::FromString(ChannelName), FText::AsNumber(Index)));
            }
            if (!FMath::IsFinite(Entry.SelectionWeight) || Entry.SelectionWeight <= 0.0f) {
                OutErrors.Add(FText::Format(LOCTEXT("InvalidRewardWeight", "{0} reward row {1} selection weight must be finite and positive."),
                                            FText::FromString(ChannelName), FText::AsNumber(Index)));
            }
            if (!IsValidEnumValue(Entry.QualityPolicy) || !IsValidEnumValue(Entry.FixedQuality)) {
                OutErrors.Add(FText::Format(LOCTEXT("InvalidRewardQuality", "{0} reward row {1} has an invalid quality policy or fixed tier."),
                                            FText::FromString(ChannelName), FText::AsNumber(Index)));
            }
            if (Entry.ItemDefinition) {
                int32 QualityFragmentCount = 0;
                const UYieldQualityFragment *DefinitionQuality = nullptr;
                for (const UItemFragment *Fragment : Entry.ItemDefinition->Fragments) {
                    if (const UYieldQualityFragment *Quality =
                            Cast<UYieldQualityFragment>(Fragment)) {
                        ++QualityFragmentCount;
                        DefinitionQuality = Quality;
                    }
                }
                if (QualityFragmentCount > 1
                    || (DefinitionQuality
                        && !IsValidEnumValue(DefinitionQuality->QualityTier))
                    || (Entry.QualityPolicy
                            != EMythicHarvestRewardQualityPolicy::DefinitionDefault
                        && QualityFragmentCount != 1)) {
                    OutErrors.Add(FText::Format(
                        LOCTEXT("MissingRewardQualityFragment",
                                "{0} reward row {1} must resolve to at most one Yield Quality Fragment, and an override policy requires exactly one."),
                        FText::FromString(ChannelName), FText::AsNumber(Index)));
                }
                const bool bWouldMintRagged =
                    Entry.QualityPolicy == EMythicHarvestRewardQualityPolicy::Fixed
                        ? Entry.FixedQuality == EMythicYieldQuality::Ragged
                        : Entry.QualityPolicy
                                == EMythicHarvestRewardQualityPolicy::DefinitionDefault
                            && DefinitionQuality
                            && DefinitionQuality->QualityTier
                                == EMythicYieldQuality::Ragged;
                if (bWouldMintRagged) {
                    OutErrors.Add(FText::Format(
                        LOCTEXT("RaggedHarvestRewardQuality",
                                "{0} reward row {1} cannot author Ragged quality; Ragged is reserved for botched hunting yields."),
                        FText::FromString(ChannelName), FText::AsNumber(Index)));
                }
            }
        }
    }
} // namespace

FPrimaryAssetId UMythicHarvestableDefinition::GetPrimaryAssetId() const { return FPrimaryAssetId(UMythicAssetManager::HarvestableDefinitionType, GetFName()); }

bool UMythicHarvestableDefinition::AppendValidationErrors(TArray<FText> &OutErrors) const {
    const int32 InitialErrorCount = OutErrors.Num();

    if (DisplayName.IsEmpty()) {
        OutErrors.Add(LOCTEXT("MissingDisplayName", "Harvestable Display Name is required."));
    }
    if (HarvestVerb.IsEmpty()) {
        OutErrors.Add(LOCTEXT("MissingHarvestVerb", "Harvestable Harvest Verb is required."));
    }
    if (!RequiredToolType || MinimumToolTier < 0) {
        OutErrors.Add(LOCTEXT("InvalidToolRequirement", "Launch harvestables require one direct Tool Type and a non-negative Minimum Tool Tier; unarmed harvesting has no authority provenance contract."));
    }

    FMythicHarvestWork QuantizedMaxWork;
    if (!FMythicHarvestWork::TryFromWorkUnits(MaxWork, QuantizedMaxWork) || QuantizedMaxWork.IsZero()) {
        OutErrors.Add(LOCTEXT("InvalidMaxWork", "Max Work must be finite, positive, and representable at the fixed harvest-work quantum."));
    }

    float PreviousThreshold = 0.0f;
    for (int32 Index = 0; Index < WorkStages.Num(); ++Index) {
        const float Threshold = WorkStages[Index].CompletedWorkFraction;
        if (!FMath::IsFinite(Threshold) || Threshold <= 0.0f || Threshold >= 1.0f || Threshold <= PreviousThreshold) {
            OutErrors.Add(
                FText::Format(LOCTEXT("InvalidWorkStage", "Work Stage {0} must be finite, strictly ascending, and inside (0,1)."), FText::AsNumber(Index)));
        }
        PreviousThreshold = Threshold;
    }

    if (!ProficiencyDefinition) {
        OutErrors.Add(LOCTEXT("MissingProficiency", "Harvestable requires one direct Proficiency Definition."));
    }
    if (!FMath::IsFinite(ProficiencyXPPerAppliedWork) || ProficiencyXPPerAppliedWork < 0.0f || !FMath::IsFinite(CompletionProficiencyXP) ||
        CompletionProficiencyXP < 0.0f) {
        OutErrors.Add(LOCTEXT("InvalidProficiencyXP", "Proficiency XP per work and completion XP must be finite and non-negative."));
    }

    if (PrimaryMaterials.IsEmpty()) {
        OutErrors.Add(LOCTEXT("MissingPrimaryMaterial", "Harvestable requires at least one direct primary-material reward row."));
    }
    TSet<const UItemDefinition *> SeenRewardItems;
    AppendRewardErrors(PrimaryMaterials, TEXT("Primary material"), SeenRewardItems, OutErrors);
    AppendRewardErrors(BonusLoot, TEXT("Bonus loot"), SeenRewardItems, OutErrors);

    if (QuestCredit.bEmitCompletionCredit && QuestCredit.CreditCount < 1) {
        OutErrors.Add(LOCTEXT("InvalidQuestCredit", "Enabled completion quest credit requires a positive whole credit count."));
    }
    if (!FMath::IsFinite(Pressure.PressurePerCompletion) || Pressure.PressurePerCompletion < 0.0f) {
        OutErrors.Add(LOCTEXT("InvalidPressure", "Harvest pressure per completion must be finite and non-negative."));
    }
    if (!FMath::IsFinite(RespawnDelaySeconds) || RespawnDelaySeconds < 0.0f) {
        OutErrors.Add(LOCTEXT("InvalidRespawnDelay", "Respawn Delay must be finite and non-negative seconds."));
    }

    return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UMythicHarvestableDefinition::IsDataValid(FDataValidationContext &Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    AppendValidationErrors(Errors);
    for (const FText &Error : Errors) {
        Context.AddError(Error);
    }
    return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
