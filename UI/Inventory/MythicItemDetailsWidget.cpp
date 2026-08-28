// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicItemDetailsWidget.h"

#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
#include "UI/Inventory/MythicAffixRowWidget.h"
#include "UI/Inventory/MythicDPSWidget.h"

namespace {

struct FPendingAffixRow {
    UMythicAffixRowWidget *Widget = nullptr;
    UPanelWidget *TargetPanel = nullptr;
};

} // namespace

void UMythicItemDetailsWidget::RefreshAffixSection(UMythicItemInstance *Item) {
    CancelPendingAffixRefresh();
    ClearItemStatSections();
    RequestedItem = Item;

    if (!Item) {
        UnbindItemizationRegistry();
        SetAffixSectionState(EMythicAffixSectionState::Empty,
                             NSLOCTEXT("MythicItemDetails", "NoItemStats", "No item stats"));
        return;
    }

    UGameInstance *GameInstance = GetGameInstance();
    UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>()
        : nullptr;
    if (!Registry) {
        UnbindItemizationRegistry();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "MissingItemStatRegistry",
                                      "Item stat data is unavailable"));
        return;
    }
    BindItemizationRegistry(Registry);

    const uint32 RequestSerial = AffixRefreshSerial;
    if (Registry->IsCoreSemanticReady()) {
        RebuildAffixRows(Item, Registry, RequestSerial);
        return;
    }

    SetAffixSectionState(EMythicAffixSectionState::Loading,
                         NSLOCTEXT("MythicItemDetails", "LoadingItemStats", "Loading item stats..."));

    const TWeakObjectPtr<UMythicItemDetailsWidget> WeakThis(this);
    const TWeakObjectPtr<UMythicItemInstance> WeakItem(Item);
    Registry->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
        [WeakThis, WeakItem, RequestSerial](const bool bReady) {
            UMythicItemDetailsWidget *Self = WeakThis.Get();
            UMythicItemInstance *Requested = WeakItem.Get();
            if (!Self || !Requested || Self->AffixRefreshSerial != RequestSerial
                || Self->RequestedItem.Get() != Requested) {
                return;
            }

            UGameInstance *ReadyGameInstance = Self->GetGameInstance();
            UMythicItemizationDataRegistrySubsystem *ReadyRegistry = ReadyGameInstance
                ? ReadyGameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>()
                : nullptr;
            if (!bReady || !ReadyRegistry || !ReadyRegistry->IsCoreSemanticReady()) {
                Self->SetAffixSectionState(
                    EMythicAffixSectionState::Error,
                    NSLOCTEXT("MythicItemDetails", "ItemStatReadinessFailed",
                              "Item stat data failed to load"));
                return;
            }

            Self->RebuildAffixRows(Requested, ReadyRegistry, RequestSerial);
        }));
}

void UMythicItemDetailsWidget::NativeDestruct() {
    CancelPendingAffixRefresh();
    UnbindItemizationRegistry();
    RequestedItem.Reset();
    ClearItemStatSections();
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

    // A quarantine notification is emitted before resident objects mutate. Invalidate both the rows and any
    // one-shot readiness callback immediately; a later committed/recovered revision rebuilds from the new graph.
    CancelPendingAffixRefresh();
    ClearItemStatSections();

    UMythicItemInstance *Item = RequestedItem.Get();
    UMythicItemizationDataRegistrySubsystem *Registry = BoundItemizationRegistry.Get();
    if (!Item) {
        SetAffixSectionState(EMythicAffixSectionState::Empty,
                             NSLOCTEXT("MythicItemDetails", "NoItemAfterSemanticRefresh", "No item stats"));
        return;
    }
    if (!Registry || !Registry->IsCoreSemanticReady()) {
        SetAffixSectionState(EMythicAffixSectionState::Loading,
                             NSLOCTEXT("MythicItemDetails", "RefreshingItemStatSemantics",
                                      "Refreshing item stats..."));
        return;
    }

    RebuildAffixRows(Item, Registry, AffixRefreshSerial);
}

void UMythicItemDetailsWidget::RebuildAffixRows(UMythicItemInstance *Item,
                                                UMythicItemizationDataRegistrySubsystem *Registry,
                                                const uint32 RequestSerial) {
    if (RequestSerial != AffixRefreshSerial || RequestedItem.Get() != Item) {
        return;
    }

    TArray<FAffixDisplayData> DisplayAffixes;
    if (!UItemTooltipVM::BuildAffixDisplayData(Item, Registry, DisplayAffixes)) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "ItemStatProjectionFailed",
                                      "Item stat presentation is unavailable"));
        return;
    }

    const bool bUseAttackPresentation =
        UItemTooltipVM::ShouldUseWeaponAttackPresentation(Item);
    FMythicWeaponAttackViewData AttackDisplayData;
    if (bUseAttackPresentation
        && !UItemTooltipVM::BuildWeaponAttackDisplayData(
            Item, Registry, DisplayAffixes, AttackDisplayData)) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "AttackProjectionFailed",
                                      "Weapon attack presentation is unavailable"));
        return;
    }

    TArray<const FAffixDisplayData *> RoutedAffixes;
    RoutedAffixes.Reserve(DisplayAffixes.Num());
    for (const FAffixDisplayData &DisplayData : DisplayAffixes) {
        if (!DisplayData.bOwnedByWeaponAttackPresentation) {
            RoutedAffixes.Add(&DisplayData);
        }
    }

    if (RoutedAffixes.IsEmpty() && !bUseAttackPresentation) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Empty,
                             NSLOCTEXT("MythicItemDetails", "ItemHasNoStats", "No item stats"));
        return;
    }

    if (!RoutedAffixes.IsEmpty() && !AffixRowClass) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "MissingAffixRowClass",
                                      "Affix row presentation is not configured"));
        return;
    }

    const bool bNeedsCoreStats = RoutedAffixes.ContainsByPredicate([](const FAffixDisplayData *DisplayData) {
        return DisplayData && DisplayData->ViewData.SourceKind == AFFIX_SOURCE_IMPLICIT;
    });
    const bool bNeedsAffixes = RoutedAffixes.ContainsByPredicate([](const FAffixDisplayData *DisplayData) {
        return DisplayData && DisplayData->ViewData.SourceKind != AFFIX_SOURCE_IMPLICIT;
    });
    if ((bNeedsCoreStats && !CoreStats) || (bNeedsAffixes && !AffixesContainer)) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "MissingAffixPanel",
                                      "Affix layout is not configured"));
        return;
    }

    TArray<FPendingAffixRow> PendingRows;
    PendingRows.Reserve(RoutedAffixes.Num());
    for (const FAffixDisplayData *DisplayData : RoutedAffixes) {
        if (!DisplayData) {
            ClearItemStatSections();
            SetAffixSectionState(EMythicAffixSectionState::Error,
                                 NSLOCTEXT("MythicItemDetails", "InvalidAffixRoute",
                                          "Affix presentation is unavailable"));
            return;
        }

        UPanelWidget *TargetPanel = DisplayData->ViewData.SourceKind == AFFIX_SOURCE_IMPLICIT
            ? CoreStats.Get()
            : AffixesContainer.Get();
        if (!TargetPanel) {
            ClearItemStatSections();
            SetAffixSectionState(EMythicAffixSectionState::Error,
                                 NSLOCTEXT("MythicItemDetails", "MissingAffixRoute",
                                          "Affix layout is not configured"));
            return;
        }

        UMythicAffixRowWidget *Row = CreateWidget<UMythicAffixRowWidget>(this, AffixRowClass);
        if (!Row) {
            ClearItemStatSections();
            SetAffixSectionState(EMythicAffixSectionState::Error,
                                 NSLOCTEXT("MythicItemDetails", "AffixRowCreationFailed",
                                          "Affix presentation could not be created"));
            return;
        }

        Row->SetFromAffixDisplayData(*DisplayData);
        FPendingAffixRow &Pending = PendingRows.AddDefaulted_GetRef();
        Pending.Widget = Row;
        Pending.TargetPanel = TargetPanel;
    }

    if (bUseAttackPresentation && !EnsureDPSWidget()) {
        ClearItemStatSections();
        SetAffixSectionState(EMythicAffixSectionState::Error,
                             NSLOCTEXT("MythicItemDetails", "MissingAttackPresentation",
                                      "Weapon attack layout is not configured"));
        return;
    }

    ClearAffixPanels();
    for (const FPendingAffixRow &Pending : PendingRows) {
        Pending.TargetPanel->AddChild(Pending.Widget);
    }
    if (bUseAttackPresentation) {
        ActiveDPSWidget->SetAttackDisplayData(AttackDisplayData);
        DPS_Slot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    SetAffixSectionState(EMythicAffixSectionState::Ready);
}

void UMythicItemDetailsWidget::SetAffixSectionState(const EMythicAffixSectionState NewState,
                                                    const FText &Message) {
    AffixSectionState = NewState;
    UpdateAffixPanelVisibility();
    OnAffixSectionStateChanged(NewState, Message);
}

bool UMythicItemDetailsWidget::EnsureDPSWidget() {
    if (!DPS_Slot || !DPSWidgetClass) {
        return false;
    }

    if (!ActiveDPSWidget) {
        ActiveDPSWidget = CreateWidget<UMythicDPSWidget>(this, DPSWidgetClass);
        if (!ActiveDPSWidget) {
            return false;
        }
    }

    if (ActiveDPSWidget->GetParent() != DPS_Slot) {
        DPS_Slot->ClearChildren();
        if (!DPS_Slot->AddChild(ActiveDPSWidget)) {
            ActiveDPSWidget = nullptr;
            return false;
        }
    }
    return true;
}

void UMythicItemDetailsWidget::ClearAttackPresentation() {
    if (ActiveDPSWidget) {
        ActiveDPSWidget->ClearAttackDisplayData();
    }
    if (DPS_Slot) {
        DPS_Slot->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicItemDetailsWidget::ClearItemStatSections() {
    ClearAttackPresentation();
    ClearAffixPanels();
}

void UMythicItemDetailsWidget::ClearAffixPanels() const {
    if (AffixesContainer) {
        AffixesContainer->ClearChildren();
        AffixesContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (CoreStats) {
        CoreStats->ClearChildren();
        CoreStats->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicItemDetailsWidget::UpdateAffixPanelVisibility() const {
    const auto UpdatePanel = [this](UPanelWidget *Panel) {
        if (!Panel) {
            return;
        }

        const bool bShow = AffixSectionState == EMythicAffixSectionState::Ready
            && Panel->GetChildrenCount() > 0;
        Panel->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
