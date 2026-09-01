// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicItemDetailsWidget.h"

#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ViewModels/ItemComparisonVM.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
#include "Itemization/Inventory/ViewModels/MythicStatDelta.h"
#include "UI/Inventory/MythicAffixRowWidget.h"
#include "UI/Inventory/MythicDPSWidget.h"

namespace {

enum class EAffixPanelRoute : uint8 {
    CoreStats,
    Affixes
};

struct FPendingAffixRow {
    FMythicAffixRowPresentation Presentation;
    EAffixPanelRoute Route = EAffixPanelRoute::Affixes;
};

bool IsAffixRoutedToCoreStats(const FAffixDisplayData &DisplayData) {
    return DisplayData.ViewData.SourceKind == AFFIX_SOURCE_IMPLICIT;
}

void AppendComparableAffixStats(
    const TConstArrayView<FAffixDisplayData> Affixes,
    TArray<FMythicComparableStat> &OutStats) {
    for (const FAffixDisplayData &Affix : Affixes) {
        if (Affix.bOwnedByWeaponAttackPresentation) {
            continue;
        }
        for (const FMythicAffixValueViewData &Value : Affix.ViewData.Values) {
            OutStats.Emplace(
                Value.StatTag,
                Value.StatLabel,
                Value.ComparisonValue,
                Value.ContributionIdentity,
                Value.ComparisonDirection,
                Value.NumberPresentation);
        }
    }
}

FText BuildRowAccessibleSummary(const TConstArrayView<FAttributeDiff> Diffs) {
    TArray<FText> Parts;
    Parts.Reserve(Diffs.Num());
    for (const FAttributeDiff &Diff : Diffs) {
        if (!Diff.AccessibleSummary.IsEmpty()) {
            Parts.Add(Diff.AccessibleSummary);
        }
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(
            NSLOCTEXT("MythicItemDetails", "AffixAccessibleSeparator", " "),
            Parts);
}

void BuildAffixRowPresentations(
    const TConstArrayView<FAffixDisplayData> CandidateAffixes,
    const TConstArrayView<FAffixDisplayData> BaselineAffixes,
    const bool bComparisonActive,
    TArray<FPendingAffixRow> &OutRows) {
    OutRows.Reset();

    TArray<FAttributeDiff> Diffs;
    TMap<FGameplayTag, const FAttributeDiff *> DiffByTag;
    if (bComparisonActive) {
        TArray<FMythicComparableStat> CandidateStats;
        TArray<FMythicComparableStat> BaselineStats;
        AppendComparableAffixStats(CandidateAffixes, CandidateStats);
        AppendComparableAffixStats(BaselineAffixes, BaselineStats);
        Diffs = FMythicStatDeltaCore::ComputeDiffs(CandidateStats, BaselineStats);
        DiffByTag.Reserve(Diffs.Num());
        for (const FAttributeDiff &Diff : Diffs) {
            DiffByTag.Add(Diff.ComparisonTag, &Diff);
        }
    }

    TSet<FGameplayTag> ClaimedCandidateDiffs;
    ClaimedCandidateDiffs.Reserve(Diffs.Num());
    for (const FAffixDisplayData &CandidateAffix : CandidateAffixes) {
        if (CandidateAffix.bOwnedByWeaponAttackPresentation) {
            continue;
        }

        FPendingAffixRow &Pending = OutRows.AddDefaulted_GetRef();
        Pending.Presentation.DisplayData = CandidateAffix;
        Pending.Route = IsAffixRoutedToCoreStats(CandidateAffix)
            ? EAffixPanelRoute::CoreStats
            : EAffixPanelRoute::Affixes;

        if (bComparisonActive) {
            for (const FMythicAffixValueViewData &Channel : CandidateAffix.ViewData.Values) {
                const FAttributeDiff *Found = DiffByTag.FindRef(Channel.StatTag);
                if (Found && !ClaimedCandidateDiffs.Contains(Channel.StatTag)) {
                    Pending.Presentation.ValueDiffs.Add(*Found);
                    ClaimedCandidateDiffs.Add(Channel.StatTag);
                }
            }
            Pending.Presentation.AccessibleSummary = BuildRowAccessibleSummary(
                Pending.Presentation.ValueDiffs);
        }
    }

    if (!bComparisonActive) {
        return;
    }

    TSet<FGameplayTag> ClaimedBaselineOnlyDiffs;
    ClaimedBaselineOnlyDiffs.Reserve(Diffs.Num());
    for (const FAffixDisplayData &BaselineAffix : BaselineAffixes) {
        if (BaselineAffix.bOwnedByWeaponAttackPresentation) {
            continue;
        }

        FMythicAffixRowPresentation Synthetic;
        Synthetic.DisplayData = BaselineAffix;
        for (const FMythicAffixValueViewData &Channel : BaselineAffix.ViewData.Values) {
            const FAttributeDiff *Found = DiffByTag.FindRef(Channel.StatTag);
            if (Found && Found->bBaselineOnly
                && Found->Movement != EMythicStatValueMovement::Equal
                && !Found->FormattedDelta.IsEmpty()
                && !ClaimedBaselineOnlyDiffs.Contains(Channel.StatTag)) {
                Synthetic.ValueDiffs.Add(*Found);
                ClaimedBaselineOnlyDiffs.Add(Channel.StatTag);
            }
        }
        if (Synthetic.ValueDiffs.IsEmpty()) {
            continue;
        }

        Synthetic.bBaselineOnly = true;
        Synthetic.AccessibleSummary = BuildRowAccessibleSummary(Synthetic.ValueDiffs);
        FPendingAffixRow &Pending = OutRows.AddDefaulted_GetRef();
        Pending.Presentation = MoveTemp(Synthetic);
        Pending.Route = IsAffixRoutedToCoreStats(BaselineAffix)
            ? EAffixPanelRoute::CoreStats
            : EAffixPanelRoute::Affixes;
    }
}

} // namespace

void UMythicItemDetailsWidget::NativeConstruct() {
    Super::NativeConstruct();
    UpdateComparisonContextPresentation();
}

void UMythicItemDetailsWidget::PresentItemStatSections(
    UMythicItemInstance *CandidateItem,
    const FMythicItemDetailsComparisonContext &ComparisonContext) {
    CancelPendingAffixRefresh();
    const uint32 RequestSerial = AffixRefreshSerial;
    ClearItemStatSections();
    if (RequestSerial != AffixRefreshSerial) {
        return;
    }

    RequestedCandidateItem = CandidateItem;
    RequestedCandidateGuid = CandidateItem
        ? CandidateItem->GetItemInstanceGuid()
        : FGuid();
    RequestedTargetLabel = ComparisonContext.TargetLabel;
    RequestedTargetSlotIndex = ComparisonContext.TargetSlotIndex;
    bRequestedTargetEmpty = ComparisonContext.bTargetEmpty;
    bRequestedCanCycleTarget = ComparisonContext.bCanCycleTarget;
    bRequestedComparisonActive = false;
    RequestedBaselineItem.Reset();
    RequestedBaselineGuid.Invalidate();

    UMythicItemInstance *BaselineItem = ComparisonContext.BaselineItem.Get();
    const bool bValidOccupiedComparison = CandidateItem
        && RequestedCandidateGuid.IsValid()
        && ComparisonContext.bComparisonActive
        && !ComparisonContext.bTargetEmpty
        && ComparisonContext.TargetSlotIndex != INDEX_NONE
        && !ComparisonContext.TargetLabel.IsEmpty()
        && BaselineItem
        && BaselineItem != CandidateItem
        && ComparisonContext.ExpectedBaselineGuid.IsValid()
        && BaselineItem->GetItemInstanceGuid() == ComparisonContext.ExpectedBaselineGuid
        && BaselineItem->GetItemInstanceGuid() != RequestedCandidateGuid;
    if (bValidOccupiedComparison) {
        bRequestedComparisonActive = true;
        RequestedBaselineItem = BaselineItem;
        RequestedBaselineGuid = ComparisonContext.ExpectedBaselineGuid;
    }
    UpdateComparisonContextPresentation();
    if (RequestSerial != AffixRefreshSerial) {
        return;
    }

    if (!CandidateItem) {
        UnbindItemizationRegistry();
        SetAffixSectionState(
            EMythicAffixSectionState::Empty,
            NSLOCTEXT("MythicItemDetails", "NoItemStats", "No item stats"));
        return;
    }
    if (!RequestedCandidateGuid.IsValid()) {
        UnbindItemizationRegistry();
        SetAffixSectionState(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "MissingItemIdentity",
                      "Item identity is unavailable"));
        return;
    }

    UGameInstance *GameInstance = GetGameInstance();
    UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>()
        : nullptr;
    if (!Registry) {
        UnbindItemizationRegistry();
        SetAffixSectionState(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "MissingItemStatRegistry",
                      "Item stat data is unavailable"));
        return;
    }

    BindItemizationRegistry(Registry);
    BeginSemanticReadyPresentation();
}

void UMythicItemDetailsWidget::ClearPresentedItem() {
    CancelPendingAffixRefresh();
    const uint32 RequestSerial = AffixRefreshSerial;
    UnbindItemizationRegistry();
    RequestedCandidateItem.Reset();
    RequestedBaselineItem.Reset();
    RequestedCandidateGuid.Invalidate();
    RequestedBaselineGuid.Invalidate();
    RequestedTargetLabel = FText::GetEmpty();
    RequestedTargetSlotIndex = INDEX_NONE;
    bRequestedComparisonActive = false;
    bRequestedTargetEmpty = true;
    bRequestedCanCycleTarget = false;
    ClearItemStatSections();
    if (RequestSerial != AffixRefreshSerial) {
        return;
    }
    UpdateComparisonContextPresentation();
    if (RequestSerial != AffixRefreshSerial) {
        return;
    }
    SetAffixSectionState(
        EMythicAffixSectionState::Empty,
        NSLOCTEXT("MythicItemDetails", "NoPresentedItem", "No item stats"));
}

bool UMythicItemDetailsWidget::ScrollDetailsBy(const float SlateUnits) {
    if (!DetailsScrollBox || !FMath::IsFinite(SlateUnits)
        || FMath::IsNearlyZero(SlateUnits)) {
        return false;
    }
    DetailsScrollBox->SetScrollOffset(
        FMath::Max(0.0f, DetailsScrollBox->GetScrollOffset() + SlateUnits));
    return true;
}

void UMythicItemDetailsWidget::RefreshAffixSection(UMythicItemInstance *Item) {
    PresentItemStatSections(Item, FMythicItemDetailsComparisonContext());
}

void UMythicItemDetailsWidget::NativeDestruct() {
    CancelPendingAffixRefresh();
    UnbindItemizationRegistry();
    RequestedCandidateItem.Reset();
    RequestedBaselineItem.Reset();
    RequestedCandidateGuid.Invalidate();
    RequestedBaselineGuid.Invalidate();
    RequestedTargetLabel = FText::GetEmpty();
    RequestedTargetSlotIndex = INDEX_NONE;
    bRequestedComparisonActive = false;
    bRequestedTargetEmpty = true;
    bRequestedCanCycleTarget = false;
    ClearItemStatSections();
    AffixSectionState = EMythicAffixSectionState::Empty;
    Super::NativeDestruct();
}

void UMythicItemDetailsWidget::BindItemizationRegistry(
    UMythicItemizationDataRegistrySubsystem *Registry) {
    if (BoundItemizationRegistry.Get() == Registry && SemanticDataChangedHandle.IsValid()) {
        return;
    }

    UnbindItemizationRegistry();
    if (!Registry) {
        return;
    }

    BoundItemizationRegistry = Registry;
    SemanticDataChangedHandle = Registry->OnSemanticDataChanged().AddUObject(
        this, &ThisClass::HandleSemanticDataChanged);
}

void UMythicItemDetailsWidget::UnbindItemizationRegistry() {
    if (UMythicItemizationDataRegistrySubsystem *Registry = BoundItemizationRegistry.Get();
        Registry && SemanticDataChangedHandle.IsValid()) {
        Registry->OnSemanticDataChanged().Remove(SemanticDataChangedHandle);
    }
    SemanticDataChangedHandle.Reset();
    BoundItemizationRegistry.Reset();
}

void UMythicItemDetailsWidget::HandleSemanticDataChanged(const uint64 SemanticRevision) {
    (void)SemanticRevision;
    CancelPendingAffixRefresh();
    const uint32 RequestSerial = AffixRefreshSerial;
    ClearItemStatSections();
    if (RequestSerial != AffixRefreshSerial) {
        return;
    }

    if (!RequestedCandidateItem.IsValid()) {
        SetAffixSectionState(
            EMythicAffixSectionState::Empty,
            NSLOCTEXT("MythicItemDetails", "NoItemAfterSemanticRefresh", "No item stats"));
        return;
    }
    BeginSemanticReadyPresentation();
}

void UMythicItemDetailsWidget::BeginSemanticReadyPresentation() {
    UMythicItemInstance *CandidateItem = RequestedCandidateItem.Get();
    UMythicItemInstance *BaselineItem = bRequestedComparisonActive
        ? RequestedBaselineItem.Get()
        : nullptr;
    if (bRequestedComparisonActive
        && (!BaselineItem
            || !RequestedBaselineGuid.IsValid()
            || BaselineItem->GetItemInstanceGuid() != RequestedBaselineGuid
            || BaselineItem->GetItemInstanceGuid() == RequestedCandidateGuid)) {
        bRequestedComparisonActive = false;
        RequestedBaselineItem.Reset();
        RequestedBaselineGuid.Invalidate();
        BaselineItem = nullptr;
        UpdateComparisonContextPresentation();
    }
    UMythicItemizationDataRegistrySubsystem *Registry = BoundItemizationRegistry.Get();
    const uint32 RequestSerial = AffixRefreshSerial;
    if (!CandidateItem || !Registry
        || !IsPresentationRequestCurrent(CandidateItem, BaselineItem, RequestSerial)) {
        return;
    }

    if (Registry->IsCoreSemanticReady()) {
        const uint64 SemanticRevision = Registry->GetSemanticDataRevision();
        RebuildItemStatSections(
            CandidateItem, BaselineItem, Registry, RequestSerial, SemanticRevision);
        return;
    }

    SetAffixSectionState(
        EMythicAffixSectionState::Loading,
        NSLOCTEXT("MythicItemDetails", "LoadingItemStats", "Loading item stats..."));

    const TWeakObjectPtr<UMythicItemDetailsWidget> WeakThis(this);
    const TWeakObjectPtr<UMythicItemInstance> WeakCandidate(CandidateItem);
    const TWeakObjectPtr<UMythicItemInstance> WeakBaseline(BaselineItem);
    Registry->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
        [WeakThis, WeakCandidate, WeakBaseline, RequestSerial](const bool bReady) {
            UMythicItemDetailsWidget *Self = WeakThis.Get();
            UMythicItemInstance *Candidate = WeakCandidate.Get();
            UMythicItemInstance *Baseline = WeakBaseline.Get();
            if (!Self || RequestSerial != Self->AffixRefreshSerial) {
                return;
            }
            if (!Candidate) {
                Self->ClearPresentedItem();
                return;
            }
            if (Self->RequestedCandidateItem.Get() != Candidate
                || !Self->RequestedCandidateGuid.IsValid()
                || Candidate->GetItemInstanceGuid() != Self->RequestedCandidateGuid) {
                return;
            }

            if (Self->bRequestedComparisonActive
                && (!Baseline
                    || !Self->RequestedBaselineGuid.IsValid()
                    || Baseline->GetItemInstanceGuid() != Self->RequestedBaselineGuid
                    || Baseline->GetItemInstanceGuid() == Self->RequestedCandidateGuid)) {
                Self->bRequestedComparisonActive = false;
                Self->RequestedBaselineItem.Reset();
                Self->RequestedBaselineGuid.Invalidate();
                Baseline = nullptr;
                Self->UpdateComparisonContextPresentation();
            }
            if (!Self->IsPresentationRequestCurrent(
                    Candidate, Baseline, RequestSerial)) {
                return;
            }

            UMythicItemizationDataRegistrySubsystem *ReadyRegistry =
                Self->BoundItemizationRegistry.Get();
            if (!bReady || !ReadyRegistry || !ReadyRegistry->IsCoreSemanticReady()) {
                Self->SetAffixSectionState(
                    EMythicAffixSectionState::Error,
                    NSLOCTEXT("MythicItemDetails", "ItemStatReadinessFailed",
                              "Item stat data failed to load"));
                return;
            }

            const uint64 SemanticRevision = ReadyRegistry->GetSemanticDataRevision();
            Self->RebuildItemStatSections(
                Candidate, Baseline, ReadyRegistry, RequestSerial, SemanticRevision);
        }));
}

bool UMythicItemDetailsWidget::IsPresentationRequestCurrent(
    const UMythicItemInstance *CandidateItem,
    const UMythicItemInstance *BaselineItem,
    const uint32 RequestSerial) const {
    if (RequestSerial != AffixRefreshSerial
        || !CandidateItem
        || RequestedCandidateItem.Get() != CandidateItem
        || !RequestedCandidateGuid.IsValid()
        || CandidateItem->GetItemInstanceGuid() != RequestedCandidateGuid) {
        return false;
    }
    if (!bRequestedComparisonActive) {
        return BaselineItem == nullptr;
    }
    return BaselineItem
        && RequestedBaselineItem.Get() == BaselineItem
        && RequestedBaselineGuid.IsValid()
        && BaselineItem->GetItemInstanceGuid() == RequestedBaselineGuid
        && BaselineItem->GetItemInstanceGuid() != RequestedCandidateGuid;
}

FMythicItemDetailsComparisonContext
UMythicItemDetailsWidget::MakeCurrentComparisonContext() const {
    FMythicItemDetailsComparisonContext Context;
    Context.bComparisonActive = bRequestedComparisonActive;
    Context.TargetLabel = RequestedTargetLabel;
    Context.bTargetEmpty = bRequestedTargetEmpty;
    Context.BaselineItem = bRequestedComparisonActive
        ? RequestedBaselineItem.Get()
        : nullptr;
    Context.ExpectedBaselineGuid = bRequestedComparisonActive
        ? RequestedBaselineGuid
        : FGuid();
    Context.TargetSlotIndex = RequestedTargetSlotIndex;
    Context.bCanCycleTarget = bRequestedCanCycleTarget;
    return Context;
}

void UMythicItemDetailsWidget::UpdateComparisonContextPresentation() {
    const FMythicItemDetailsComparisonContext Context = MakeCurrentComparisonContext();
    OnComparisonContextUpdated(Context);
}

void UMythicItemDetailsWidget::RebuildItemStatSections(
    UMythicItemInstance *CandidateItem,
    UMythicItemInstance *BaselineItem,
    UMythicItemizationDataRegistrySubsystem *Registry,
    const uint32 RequestSerial,
    const uint64 SemanticRevision) {
    const auto IsCommitCurrent = [this, CandidateItem, BaselineItem, Registry,
                                  RequestSerial, SemanticRevision]() {
        return Registry
            && Registry->IsCoreSemanticReady()
            && Registry->GetSemanticDataRevision() == SemanticRevision
            && IsPresentationRequestCurrent(CandidateItem, BaselineItem, RequestSerial);
    };
    const auto FailCurrentPresentation = [this, &IsCommitCurrent](
        const EMythicAffixSectionState FailureState,
        const FText &Message) {
        if (!IsCommitCurrent()) {
            return;
        }
        ClearItemStatSections();
        if (IsCommitCurrent()) {
            SetAffixSectionState(FailureState, Message);
        }
    };

    if (!IsCommitCurrent()) {
        return;
    }

    TArray<FAffixDisplayData> CandidateAffixes;
    if (!UItemTooltipVM::BuildAffixDisplayData(
            CandidateItem, Registry, CandidateAffixes)) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "ItemStatProjectionFailed",
                      "Item stat presentation is unavailable"));
        return;
    }

    const bool bUseCandidateAttackPresentation =
        UItemTooltipVM::ShouldUseWeaponAttackPresentation(CandidateItem);
    FMythicWeaponAttackViewData CandidateAttack;
    if (bUseCandidateAttackPresentation
        && !UItemTooltipVM::BuildWeaponAttackDisplayData(
            CandidateItem, Registry, CandidateAffixes, CandidateAttack)) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "AttackProjectionFailed",
                      "Weapon attack presentation is unavailable"));
        return;
    }

    bool bAffixComparisonReady = bRequestedComparisonActive && BaselineItem;
    bool bBaselineAttackProjectionReady = false;
    TArray<FAffixDisplayData> BaselineAffixes;
    FMythicWeaponAttackViewData BaselineAttack;
    if (bAffixComparisonReady) {
        bAffixComparisonReady = UItemTooltipVM::BuildAffixDisplayData(
            BaselineItem, Registry, BaselineAffixes);
        const bool bUseBaselineAttackPresentation =
            UItemTooltipVM::ShouldUseWeaponAttackPresentation(BaselineItem);
        if (bAffixComparisonReady && bUseBaselineAttackPresentation) {
            bBaselineAttackProjectionReady = UItemTooltipVM::BuildWeaponAttackDisplayData(
                BaselineItem, Registry, BaselineAffixes, BaselineAttack);
        }
    }

    if (!IsCommitCurrent()) {
        return;
    }

    TArray<FPendingAffixRow> PendingRows;
    BuildAffixRowPresentations(
        CandidateAffixes,
        BaselineAffixes,
        bAffixComparisonReady,
        PendingRows);

    if (PendingRows.IsEmpty() && !bUseCandidateAttackPresentation) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Empty,
            NSLOCTEXT("MythicItemDetails", "ItemHasNoStats", "No item stats"));
        return;
    }
    if (!PendingRows.IsEmpty() && !AffixRowClass) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "MissingAffixRowClass",
                      "Affix row presentation is not configured"));
        return;
    }

    const bool bNeedsCoreStats = PendingRows.ContainsByPredicate(
        [](const FPendingAffixRow &Row) {
            return Row.Route == EAffixPanelRoute::CoreStats;
        });
    const bool bNeedsAffixes = PendingRows.ContainsByPredicate(
        [](const FPendingAffixRow &Row) {
            return Row.Route == EAffixPanelRoute::Affixes;
        });
    if ((bNeedsCoreStats && !CoreStats) || (bNeedsAffixes && !AffixesContainer)) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "MissingAffixPanel",
                      "Affix layout is not configured"));
        return;
    }
    const bool bAttackWidgetReady = !bUseCandidateAttackPresentation || EnsureDPSWidget();
    if (!IsCommitCurrent()) {
        return;
    }
    if (!bAttackWidgetReady) {
        FailCurrentPresentation(
            EMythicAffixSectionState::Error,
            NSLOCTEXT("MythicItemDetails", "MissingAttackPresentation",
                      "Weapon attack layout is not configured"));
        return;
    }

    for (int32 Index = 0; Index < PendingRows.Num(); ++Index) {
        UMythicAffixRowWidget *Row = AcquireAffixRow(Index);
        if (!IsCommitCurrent()) {
            return;
        }
        if (!Row) {
            FailCurrentPresentation(
                EMythicAffixSectionState::Error,
                NSLOCTEXT("MythicItemDetails", "AffixRowCreationFailed",
                          "Affix presentation could not be created"));
            return;
        }
    }

    ClearAffixPanels();
    if (!IsCommitCurrent()) {
        return;
    }
    for (int32 Index = 0; Index < PendingRows.Num(); ++Index) {
        UMythicAffixRowWidget *Row = AffixRowPool[Index];
        UPanelWidget *TargetPanel = PendingRows[Index].Route == EAffixPanelRoute::CoreStats
            ? CoreStats.Get()
            : AffixesContainer.Get();
        auto *AttachedSlot = Row && TargetPanel
            ? TargetPanel->AddChild(Row)
            : nullptr;
        if (!IsCommitCurrent()) {
            return;
        }
        if (!AttachedSlot) {
            FailCurrentPresentation(
                EMythicAffixSectionState::Error,
                NSLOCTEXT("MythicItemDetails", "AffixRowAttachFailed",
                          "Affix presentation could not be attached"));
            return;
        }
        ActiveAffixRowCount = Index + 1;
        Row->SetPresentation(PendingRows[Index].Presentation);
        if (!IsCommitCurrent()) {
            return;
        }
        Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (bUseCandidateAttackPresentation) {
        ActiveDPSWidget->SetAttackDisplayData(CandidateAttack);
        if (!IsCommitCurrent()) {
            return;
        }
        if (bAffixComparisonReady && bBaselineAttackProjectionReady
            && BaselineAttack.bIsValid) {
            ActiveDPSWidget->SetAttackComparisonData(
                UItemComparisonVM::BuildWeaponAttackComparison(
                    CandidateAttack, BaselineAttack));
        }
        else {
            ActiveDPSWidget->ClearAttackComparisonData();
        }
        if (!IsCommitCurrent()) {
            return;
        }
        DPS_Slot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    if (!IsCommitCurrent()) {
        return;
    }
    SetAffixSectionState(EMythicAffixSectionState::Ready);
}

void UMythicItemDetailsWidget::SetAffixSectionState(
    const EMythicAffixSectionState NewState,
    const FText &Message) {
    AffixSectionState = NewState;
    UpdateAffixPanelVisibility();
    OnAffixSectionStateChanged(NewState, Message);
}

bool UMythicItemDetailsWidget::EnsureDPSWidget() {
    const uint32 CreationSerial = AffixRefreshSerial;
    if (!DPS_Slot || !DPSWidgetClass) {
        return false;
    }

    if (!ActiveDPSWidget || !ActiveDPSWidget->IsA(DPSWidgetClass.Get())) {
        if (ActiveDPSWidget) {
            ActiveDPSWidget->RemoveFromParent();
            if (CreationSerial != AffixRefreshSerial) {
                return false;
            }
        }
        UMythicDPSWidget *NewWidget = CreateWidget<UMythicDPSWidget>(this, DPSWidgetClass);
        if (CreationSerial != AffixRefreshSerial) {
            return false;
        }
        ActiveDPSWidget = NewWidget;
        if (!NewWidget) {
            return false;
        }
    }

    if (ActiveDPSWidget->GetParent() != DPS_Slot) {
        DPS_Slot->ClearChildren();
        if (CreationSerial != AffixRefreshSerial) {
            return false;
        }
        const bool bAttached = DPS_Slot->AddChild(ActiveDPSWidget) != nullptr;
        if (CreationSerial != AffixRefreshSerial) {
            return false;
        }
        if (!bAttached) {
            ActiveDPSWidget = nullptr;
            return false;
        }
    }
    return true;
}

UMythicAffixRowWidget *UMythicItemDetailsWidget::AcquireAffixRow(
    const int32 PoolIndex) {
    const uint32 CreationSerial = AffixRefreshSerial;
    if (PoolIndex < 0 || !AffixRowClass) {
        return nullptr;
    }
    while (AffixRowPool.Num() <= PoolIndex) {
        AffixRowPool.Add(nullptr);
    }

    UMythicAffixRowWidget *Row = AffixRowPool[PoolIndex];
    if (Row && !Row->IsA(AffixRowClass.Get())) {
        Row->RemoveFromParent();
        if (CreationSerial != AffixRefreshSerial) {
            return nullptr;
        }
        Row = nullptr;
        AffixRowPool[PoolIndex] = nullptr;
    }
    if (!Row) {
        Row = CreateWidget<UMythicAffixRowWidget>(this, AffixRowClass);
        if (CreationSerial != AffixRefreshSerial) {
            return nullptr;
        }
        AffixRowPool[PoolIndex] = Row;
    }
    return Row;
}

void UMythicItemDetailsWidget::ClearAttackPresentation() {
    const uint32 ClearSerial = AffixRefreshSerial;
    if (ActiveDPSWidget) {
        ActiveDPSWidget->ClearAttackDisplayData();
    }
    if (ClearSerial != AffixRefreshSerial) {
        return;
    }
    if (DPS_Slot) {
        DPS_Slot->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicItemDetailsWidget::ClearItemStatSections() {
    const uint32 ClearSerial = AffixRefreshSerial;
    ClearAttackPresentation();
    if (ClearSerial != AffixRefreshSerial) {
        return;
    }
    ClearAffixPanels();
}

void UMythicItemDetailsWidget::ClearAffixPanels() {
    const uint32 ClearSerial = AffixRefreshSerial;
    if (AffixesContainer) {
        AffixesContainer->ClearChildren();
        if (ClearSerial != AffixRefreshSerial) {
            return;
        }
        AffixesContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (CoreStats) {
        CoreStats->ClearChildren();
        if (ClearSerial != AffixRefreshSerial) {
            return;
        }
        CoreStats->SetVisibility(ESlateVisibility::Collapsed);
    }
    const int32 RowsToClear = ActiveAffixRowCount;
    ActiveAffixRowCount = 0;
    for (int32 Index = 0;
         Index < RowsToClear && AffixRowPool.IsValidIndex(Index);
         ++Index) {
        if (ClearSerial != AffixRefreshSerial) {
            return;
        }
        if (UMythicAffixRowWidget *Row = AffixRowPool[Index]) {
            Row->RemoveFromParent();
            Row->ClearDeltaPresentation();
            if (ClearSerial != AffixRefreshSerial) {
                return;
            }
            Row->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicItemDetailsWidget::UpdateAffixPanelVisibility() const {
    const auto UpdatePanel = [this](UPanelWidget *Panel) {
        if (!Panel) {
            return;
        }

        const bool bShow = AffixSectionState == EMythicAffixSectionState::Ready
            && Panel->GetChildrenCount() > 0;
        Panel->SetVisibility(
            bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    };

    UpdatePanel(CoreStats.Get());
    UpdatePanel(AffixesContainer.Get());
}

void UMythicItemDetailsWidget::CancelPendingAffixRefresh() {
    ++AffixRefreshSerial;
    if (AffixRefreshSerial == 0) {
        ++AffixRefreshSerial;
    }
}
