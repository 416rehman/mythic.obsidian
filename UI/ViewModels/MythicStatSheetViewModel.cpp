// Copyright Stellar Games. All Rights Reserved.

#include "UI/ViewModels/MythicStatSheetViewModel.h"

#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicStatSummary.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Settings/MythicCombatSettings.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"
#include "TimerManager.h"

namespace {
}

void UMythicStatSheetViewModel::InitializeForASC(UAbilitySystemComponent* InASC) {
    if (!InASC) {
        return;
    }

    Shutdown();
    ASC = InASC;
    BindPermanentStatLayer(InASC);

    for (UAttributeSet* Set : InASC->GetSpawnedAttributes()) {
        if (UMythicAttributeSet* MythicSet = Cast<UMythicAttributeSet>(Set)) {
            MythicSet->OnAttributeChanged.AddDynamic(this, &UMythicStatSheetViewModel::HandleAttributeChanged);
            BoundSets.Add(MythicSet);
        }
    }

    if (UWorld* World = InASC->GetWorld()) {
        if (UGameInstance* GameInstance = World->GetGameInstance()) {
            UMythicItemizationDataRegistrySubsystem* Registry =
                GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>();
            RegistrySubsystem = Registry;
            if (Registry) {
                RegistrySemanticDataChangedHandle = Registry->OnSemanticDataChanged().AddUObject(
                    this, &ThisClass::HandleSemanticDataChanged);
                if (Registry->IsCoreSemanticReady()) {
                    StatRegistry = &Registry->GetStatRegistry();
                }
                else {
                    RegistryReadinessHandle = Registry->OnReadinessChanged().AddWeakLambda(
                        this,
                        [this](EMythicItemizationReadiness) {
                            UMythicItemizationDataRegistrySubsystem* CurrentRegistry = RegistrySubsystem.Get();
                            if (!CurrentRegistry || !CurrentRegistry->IsCoreSemanticReady()) {
                                return;
                            }
                            StatRegistry = &CurrentRegistry->GetStatRegistry();
                            Rebuild();
                        });

                    Registry->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateWeakLambda(
                        this,
                        [this](bool bSuccess) {
                            UMythicItemizationDataRegistrySubsystem* CurrentRegistry = RegistrySubsystem.Get();
                            if (bSuccess && CurrentRegistry && CurrentRegistry->IsCoreSemanticReady()) {
                                StatRegistry = &CurrentRegistry->GetStatRegistry();
                                Rebuild();
                            }
                        }));
                }
            }
        }
    }

    Rebuild();
}

void UMythicStatSheetViewModel::InitializeForASCWithRegistry(
    UAbilitySystemComponent* InASC,
    const FMythicStatRegistry& InRegistry) {
    if (!InASC) {
        return;
    }

    Shutdown();
    ASC = InASC;
    StatRegistry = &InRegistry;
    BindPermanentStatLayer(InASC);
    for (UAttributeSet* Set : InASC->GetSpawnedAttributes()) {
        if (UMythicAttributeSet* MythicSet = Cast<UMythicAttributeSet>(Set)) {
            MythicSet->OnAttributeChanged.AddDynamic(this, &UMythicStatSheetViewModel::HandleAttributeChanged);
            BoundSets.Add(MythicSet);
        }
    }
    Rebuild();
}

void UMythicStatSheetViewModel::Shutdown() {
    for (const TWeakObjectPtr<UMythicAttributeSet>& WeakSet : BoundSets) {
        if (UMythicAttributeSet* Set = WeakSet.Get()) {
            Set->OnAttributeChanged.RemoveDynamic(this, &UMythicStatSheetViewModel::HandleAttributeChanged);
        }
    }
    BoundSets.Reset();

    if (UMythicItemizationDataRegistrySubsystem* Registry = RegistrySubsystem.Get()) {
        if (RegistryReadinessHandle.IsValid()) {
            Registry->OnReadinessChanged().Remove(RegistryReadinessHandle);
        }
        if (RegistrySemanticDataChangedHandle.IsValid()) {
            Registry->OnSemanticDataChanged().Remove(RegistrySemanticDataChangedHandle);
        }
    }
    RegistryReadinessHandle.Reset();
    RegistrySemanticDataChangedHandle.Reset();
    RegistrySubsystem.Reset();
    if (UMythicAffixApplicationComponent *Application = AffixApplication.Get()) {
        if (PermanentStatLayerChangedHandle.IsValid()) {
            Application->OnPermanentStatLayerChanged().Remove(PermanentStatLayerChangedHandle);
        }
    }
    PermanentStatLayerChangedHandle.Reset();
    AffixApplication.Reset();
    StatRegistry = nullptr;
    ASC = nullptr;
    bRebuildScheduled = false;
}

void UMythicStatSheetViewModel::BeginDestroy() {
    Shutdown();
    Super::BeginDestroy();
}

void UMythicStatSheetViewModel::HandleAttributeChanged(const FGameplayAttribute&, float, float) {
    ScheduleRebuild();
}

void UMythicStatSheetViewModel::BindPermanentStatLayer(UAbilitySystemComponent *InASC) {
    if (!InASC) {
        return;
    }
    UMythicAffixApplicationComponent *Application = nullptr;
    if (AActor *Owner = InASC->GetOwnerActor()) {
        Application = Owner->FindComponentByClass<UMythicAffixApplicationComponent>();
    }
    if (!Application) {
        if (AActor *Avatar = InASC->GetAvatarActor()) {
            Application = Avatar->FindComponentByClass<UMythicAffixApplicationComponent>();
        }
    }
    if (!Application) {
        return;
    }
    AffixApplication = Application;
    PermanentStatLayerChangedHandle = Application->OnPermanentStatLayerChanged().AddUObject(
        this, &ThisClass::HandlePermanentStatLayerChanged);
}

void UMythicStatSheetViewModel::HandlePermanentStatLayerChanged() {
    ScheduleRebuild();
}

void UMythicStatSheetViewModel::HandleSemanticDataChanged(
    const uint64 SemanticRevision) {
    (void)SemanticRevision;
    UMythicItemizationDataRegistrySubsystem *Registry = RegistrySubsystem.Get();
    StatRegistry = Registry && Registry->IsCoreSemanticReady()
        ? &Registry->GetStatRegistry()
        : nullptr;
    if (!StatRegistry) {
        // The registry broadcasts editor quarantine before the authored UObject mutates. Clear synchronously so a
        // visible sheet cannot retain rows backed by a semantic graph that is no longer publishable.
        Rebuild();
        return;
    }
    ScheduleRebuild();
}

void UMythicStatSheetViewModel::ScheduleRebuild() {
    if (bRebuildScheduled) {
        return;
    }

    UWorld* World = nullptr;
    if (const UAbilitySystemComponent* AbilitySystem = ASC.Get()) {
        if (const AActor* Owner = AbilitySystem->GetOwner()) {
            World = Owner->GetWorld();
        }
    }

    if (!World) {
        Rebuild();
        return;
    }

    bRebuildScheduled = true;
    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UMythicStatSheetViewModel::Rebuild));
}

void UMythicStatSheetViewModel::Refresh() {
    Rebuild();
}

TArray<FMythicStatContributionLine> UMythicStatSheetViewModel::GetContributionsFor(FGameplayAttribute Stat) const {
    TArray<FMythicStatContributionLine> Out;

    const UAbilitySystemComponent* AbilitySystem = ASC.Get();
    const UMythicCombatSettings* Settings = GetDefault<UMythicCombatSettings>();
    if (!AbilitySystem || !Settings || !Stat.IsValid() || !StatRegistry || !StatRegistry->IsBuilt()) {
        return Out;
    }

    const float StatValue = AbilitySystem->GetNumericAttribute(Stat);
    for (const FMythicStatContribution& Row : Settings->StatContributions.Contributions) {
        if (Row.SourceStat != Stat || !FMythicStatContributionRules::IsRowLive(Row)) {
            continue;
        }

        const UMythicStatDefinition* TargetDefinition = StatRegistry->FindStat(Row.TargetAttribute);
        if (!TargetDefinition) {
            continue;
        }

        FMythicStatContributionLine Line;
        Line.Label = TargetDefinition->DisplayName;
        Line.Fraction = FMythicStatContributionRules::ResolveRow(Row, StatValue);
        Line.Value = MythicStatDisplay::FormatBonus(Line.Fraction, EMythicStatFormat::Percent);

        const float Undiminished = FMath::Max(0.0f, StatValue) * Row.PerPoint;
        Line.bDiminished = Undiminished - Line.Fraction > KINDA_SMALL_NUMBER;
        Out.Add(MoveTemp(Line));
    }
    return Out;
}

void UMythicStatSheetViewModel::Rebuild() {
    bRebuildScheduled = false;

    UAbilitySystemComponent* AbilitySystem = ASC.Get();
    if (!AbilitySystem || !StatRegistry || !StatRegistry->IsBuilt()) {
        SetSections({});
        SetSummaries({});
        SetModifiedStatCount(0);
        return;
    }

    TArray<const UMythicStatCategoryDefinition*> Categories;
    StatRegistry->GetAllCategories(Categories);

    int32 ModifiedCount = 0;
    TArray<FMythicStatSection> NewSections;
    NewSections.Reserve(Categories.Num());

    for (const UMythicStatCategoryDefinition* Category : Categories) {
        if (!Category) {
            continue;
        }

        FMythicStatSection Section;
        Section.Heading = Category->DisplayName;
        Section.CategoryTag = Category->CategoryTag;
        Section.Style = Category->Style;

        for (const UMythicStatDefinition* Definition : StatRegistry->GetStatsInCategory(Category->GetPrimaryAssetId())) {
            if (!Definition || !AbilitySystem->HasAttributeSetForAttribute(Definition->Attribute)) {
                continue;
            }

            const float GasBaseValue = AbilitySystem->GetNumericAttributeBase(Definition->Attribute);
            float BaseValue = GasBaseValue;
            float EquipmentBaseValue = GasBaseValue;
            if (const UMythicAffixApplicationComponent *Application = AffixApplication.Get()) {
                Application->GetPermanentStatLayerValues(
                    Definition->Attribute, BaseValue, EquipmentBaseValue);
            }
            const float CurrentValue = AbilitySystem->GetNumericAttribute(Definition->Attribute);

            const UMythicStatDefinition* Capacity = nullptr;
            float CapacityBaseValue = 0.0f;
            float CapacityEquipmentBaseValue = 0.0f;
            float CapacityCurrentValue = 0.0f;
            bool bCapacityModified = false;
            if (Definition->PairRole == EMythicStatPairRole::Current) {
                Capacity = StatRegistry->FindStat(Definition->PairedStat.GetPrimaryAssetId());
                if (Capacity && AbilitySystem->HasAttributeSetForAttribute(Capacity->Attribute)) {
                    const float CapacityGasBaseValue = AbilitySystem->GetNumericAttributeBase(Capacity->Attribute);
                    CapacityBaseValue = CapacityGasBaseValue;
                    CapacityEquipmentBaseValue = CapacityGasBaseValue;
                    if (const UMythicAffixApplicationComponent *Application = AffixApplication.Get()) {
                        Application->GetPermanentStatLayerValues(
                            Capacity->Attribute, CapacityBaseValue, CapacityEquipmentBaseValue);
                    }
                    CapacityCurrentValue = AbilitySystem->GetNumericAttribute(Capacity->Attribute);
                    const float CapacityEpsilon = MythicStatPresentation::GetComparisonEpsilon(
                        Capacity->NumberPresentation);
                    bCapacityModified = FMath::Abs(CapacityCurrentValue - CapacityBaseValue) > CapacityEpsilon;
                }
                else {
                    Capacity = nullptr;
                }
            }

            // Hidden is an absolute authoring decision. A modified paired capacity may make a conditional
            // current row visible, but it must never resurrect a row explicitly excluded from the stat sheet.
            if (Definition->SheetVisibility == EMythicStatSheetVisibility::Hidden
                || (!MythicStatDisplay::ShouldRender(*Definition, BaseValue, CurrentValue)
                    && !bCapacityModified)) {
                continue;
            }

            FMythicStatLine Line;
            Line.StatTag = Definition->StatTag;
            Line.CategoryTag = Category->CategoryTag;
            Line.Label = Definition->DisplayName;
            Line.Description = Definition->Description;
            Line.Attribute = Definition->Attribute;
            Line.NumberPresentation = Definition->NumberPresentation;
            Line.ComparisonDirection = Definition->ComparisonDirection;
            Line.SortOrder = Definition->SheetOrder;
            Line.bEmphasizeRow = Category->Style.bEmphasizeRows;
            Line.bEnableContributionDrilldown = Category->Style.bEnableContributionDrilldown;
            Line.BaseValue = BaseValue;
            Line.EquipmentBaseValue = EquipmentBaseValue;
            Line.CurrentValue = CurrentValue;
            Line.BonusValue = CurrentValue - BaseValue;
            Line.EquipmentBonusValue = EquipmentBaseValue - BaseValue;
            Line.TemporaryBonusValue = CurrentValue - EquipmentBaseValue;

            const float Epsilon = MythicStatPresentation::GetComparisonEpsilon(Definition->NumberPresentation);
            Line.bPrimaryStatHasBonus = FMath::Abs(Line.BonusValue) > Epsilon;
            Line.PrimaryBonusText = MythicStatDisplay::FormatBonus(
                Line.BonusValue, Definition->NumberPresentation);
            Line.BonusText = Line.PrimaryBonusText;

            bool bRenderedPair = false;
            if (Capacity) {
                Line.PairedStatTag = Capacity->StatTag;
                Line.PairedAttribute = Capacity->Attribute;
                Line.PairedNumberPresentation = Capacity->NumberPresentation;
                Line.PairedBaseValue = CapacityBaseValue;
                Line.PairedEquipmentBaseValue = CapacityEquipmentBaseValue;
                Line.PairedCurrentValue = CapacityCurrentValue;
                Line.PairedBonusValue = CapacityCurrentValue - CapacityBaseValue;
                Line.PairedEquipmentBonusValue = CapacityEquipmentBaseValue - CapacityBaseValue;
                Line.PairedTemporaryBonusValue = CapacityCurrentValue - CapacityEquipmentBaseValue;
                Line.bPairedStatHasBonus = bCapacityModified;
                Line.PairedBonusText = MythicStatDisplay::FormatBonus(
                    Line.PairedBonusValue, Capacity->NumberPresentation);

                Line.Value = FText::Format(
                    NSLOCTEXT("MythicStats", "CurrentCapacity", "{0} / {1}"),
                    MythicStatDisplay::FormatValue(CurrentValue, Definition->NumberPresentation),
                    MythicStatDisplay::FormatValue(CapacityCurrentValue, Capacity->NumberPresentation));
                if (CapacityCurrentValue > 0.0f) {
                    Line.BarPercent = FMath::Clamp(CurrentValue / CapacityCurrentValue, 0.0f, 1.0f);
                }
                if (Line.bPrimaryStatHasBonus && Line.bPairedStatHasBonus) {
                    Line.BonusText = FText::Format(
                        NSLOCTEXT("MythicStats", "CurrentAndCapacityBonus", "{0} · Max {1}"),
                        Line.PrimaryBonusText, Line.PairedBonusText);
                }
                else if (Line.bPairedStatHasBonus) {
                    Line.BonusText = FText::Format(
                        NSLOCTEXT("MythicStats", "CapacityOnlyBonus", "Max {0}"),
                        Line.PairedBonusText);
                }
                bRenderedPair = true;
            }

            if (!bRenderedPair) {
                Line.Value = MythicStatDisplay::FormatValue(CurrentValue, Definition->NumberPresentation);
            }

            Line.bHasBonus = Line.bPrimaryStatHasBonus || Line.bPairedStatHasBonus;
            Line.bHasEquipmentBonus =
                FMath::Abs(Line.EquipmentBonusValue) > Epsilon
                || (Capacity && FMath::Abs(Line.PairedEquipmentBonusValue)
                    > MythicStatPresentation::GetComparisonEpsilon(Capacity->NumberPresentation));
            Line.bHasTemporaryBonus =
                FMath::Abs(Line.TemporaryBonusValue) > Epsilon
                || (Capacity && FMath::Abs(Line.PairedTemporaryBonusValue)
                    > MythicStatPresentation::GetComparisonEpsilon(Capacity->NumberPresentation));
            if (Line.bHasBonus) {
                ++ModifiedCount;
            }
            Section.Lines.Add(MoveTemp(Line));
        }

        if (Section.Lines.IsEmpty()) {
            continue;
        }
        Section.Lines.Sort([](const FMythicStatLine& Left, const FMythicStatLine& Right) {
            if (Left.SortOrder != Right.SortOrder) {
                return Left.SortOrder < Right.SortOrder;
            }
            return Left.StatTag.ToString() < Right.StatTag.ToString();
        });
        NewSections.Add(MoveTemp(Section));
    }

    TArray<FMythicStatSummaryLine> NewSummaries;
    if (const UMythicStatSummaryLibrary* Library = ResolveSummaryLibrary()) {
        NewSummaries.Reserve(Library->Summaries.Num());
        for (const UMythicStatSummaryDefinition* Definition : Library->Summaries) {
            if (!Definition) {
                continue;
            }
            FMythicStatSummaryLine Card;
            Card.SummaryId = Definition->SummaryId;
            Card.Label = Definition->Label;
            Card.Description = Definition->Description;
            Card.Icon = Definition->Icon;
            Card.RawValue = Definition->Compute(AbilitySystem);
            float RangeMinimum = 0.0f;
            float RangeMaximum = 0.0f;
            if (Definition->ComputeRange(AbilitySystem, RangeMinimum, RangeMaximum)) {
                Card.Value = FText::Format(
                    NSLOCTEXT("MythicStats", "SummaryRange", "{0} - {1}"),
                    MythicStatDisplay::FormatValue(RangeMinimum, Definition->Format),
                    MythicStatDisplay::FormatValue(RangeMaximum, Definition->Format));
            }
            else {
                Card.Value = MythicStatDisplay::FormatValue(Card.RawValue, Definition->Format);
            }
            NewSummaries.Add(MoveTemp(Card));
        }
    }

    SetSections(MoveTemp(NewSections));
    SetSummaries(MoveTemp(NewSummaries));
    SetModifiedStatCount(ModifiedCount);
}

const UMythicStatSummaryLibrary* UMythicStatSheetViewModel::ResolveSummaryLibrary() {
    if (SummaryLibrary || bSummaryLibraryTried) {
        return SummaryLibrary;
    }

    bSummaryLibraryTried = true;
    const UMythicStatDisplaySettings* Settings = GetDefault<UMythicStatDisplaySettings>();
    if (!Settings || Settings->SummaryLibrary.IsNull()) {
        return nullptr;
    }

    if (const UMythicStatSummaryLibrary* Loaded = Settings->SummaryLibrary.Get()) {
        SummaryLibrary = Loaded;
        return SummaryLibrary;
    }

    UMythicAssetManager::LoadAsync(
        this,
        Settings->SummaryLibrary,
        [this](UMythicStatSummaryLibrary* Loaded) {
            SummaryLibrary = Loaded;
            if (ASC.IsValid()) {
                Rebuild();
            }
        });
    return nullptr;
}

void UMythicStatSheetViewModel::SetSections(TArray<FMythicStatSection>&& In) {
    Sections = MoveTemp(In);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Sections);
}

void UMythicStatSheetViewModel::SetSummaries(TArray<FMythicStatSummaryLine>&& In) {
    Summaries = MoveTemp(In);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Summaries);
}

void UMythicStatSheetViewModel::SetModifiedStatCount(int32 In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ModifiedStatCount, In)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ModifiedStatCount);
    }
}
