// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/ViewModels/MythicItemComparisonTypes.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicItemDetailsWidget.generated.h"

class UMythicAffixRowWidget;
class UMythicDPSWidget;
class UMythicItemInstance;
class UMythicItemizationDataRegistrySubsystem;
class UNamedSlot;
class UPanelWidget;
class UScrollBox;

/** Presentation state for the affix portion of an item-details card. */
UENUM(BlueprintType)
enum class EMythicAffixSectionState : uint8 {
    Empty,
    Loading,
    Ready,
    Error
};

/**
 * Readiness-safe native base for WBP_ItemDetails' affix section.
 *
 * It consumes immutable item snapshots through UItemTooltipVM's canonical projection seam, and never performs a
 * synchronous semantic or presentation asset load while rebuilding the panel.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicItemDetailsWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /**
     * Presents one candidate inside this persistent ItemDetails card and optionally projects inline comparison
     * against the exact occupied target described by ComparisonContext. Both sides are built from one committed
     * semantic revision; stale identities or empty targets retain ordinary candidate presentation without deltas.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details")
    void PresentItemStatSections(
        UMythicItemInstance *CandidateItem,
        const FMythicItemDetailsComparisonContext &ComparisonContext);

    /** Clears the represented item, pooled rows, and all transient comparison presentation. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details")
    void ClearPresentedItem();

    /**
     * Moves the existing ItemDetails scroll surface by SlateUnits without changing inventory focus. Returns false
     * for an unavailable optional scroll surface or a non-finite/no-op delta.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details")
    bool ScrollDetailsBy(float SlateUnits);

    /**
     * Compatibility entry point for existing ItemDetails Blueprint graphs; forwards to ordinary no-comparison
     * presentation and never constructs a comparison view model.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details",
              meta = (DisplayName = "Refresh Item Stat Sections"))
    void RefreshAffixSection(UMythicItemInstance *Item);

    /** Current readiness and rendering state of the affix section. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Item Details")
    EMythicAffixSectionState AffixSectionState = EMythicAffixSectionState::Empty;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Non-implicit affixes are added here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> AffixesContainer;

    /** Affixes whose SourceKind is Itemization.Affix.Source.Implicit are added here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> CoreStats;

    /** Named slot reserved for the pooled dedicated weapon attack/DPS presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack", meta = (BindWidgetOptional))
    TObjectPtr<UNamedSlot> DPS_Slot;

    /** Optional existing scroll surface used by mouse wheel and gamepad right-stick forwarding. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details", meta = (BindWidgetOptional))
    TObjectPtr<UScrollBox> DetailsScrollBox;

    /** Set to the WBP_Affix subclass after its parent is migrated to UMythicAffixRowWidget. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Item Details|Affixes",
              meta = (AllowAbstract = "false"))
    TSubclassOf<UMythicAffixRowWidget> AffixRowClass;

    /** Typed presentation class instantiated once and reused whenever an Attack Fragment-bearing weapon is inspected. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Item Details|Attack",
              meta = (AllowAbstract = "false"))
    TSubclassOf<UMythicDPSWidget> DPSWidgetClass;

    /** Lets the Blueprint own loading, empty, ready, and error visuals without owning readiness control flow. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Item Details|Affixes",
              meta = (DisplayName = "On Affix Section State Changed"))
    void OnAffixSectionStateChanged(EMythicAffixSectionState NewState, const FText &Message);

    /** Lets Blueprint styling react to the typed comparison context without owning comparison or target state. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Item Details|Comparison",
              meta = (DisplayName = "On Comparison Context Updated"))
    void OnComparisonContextUpdated(
        const FMythicItemDetailsComparisonContext &ComparisonContext);

private:
    void BindItemizationRegistry(UMythicItemizationDataRegistrySubsystem *Registry);
    void UnbindItemizationRegistry();
    void HandleSemanticDataChanged(uint64 SemanticRevision);
    void BeginSemanticReadyPresentation();
    void RebuildItemStatSections(
        UMythicItemInstance *CandidateItem,
        UMythicItemInstance *BaselineItem,
        UMythicItemizationDataRegistrySubsystem *Registry,
        uint32 RequestSerial,
        uint64 SemanticRevision);
    bool IsPresentationRequestCurrent(
        const UMythicItemInstance *CandidateItem,
        const UMythicItemInstance *BaselineItem,
        uint32 RequestSerial) const;
    FMythicItemDetailsComparisonContext MakeCurrentComparisonContext() const;
    void UpdateComparisonContextPresentation();
    void SetAffixSectionState(EMythicAffixSectionState NewState, const FText &Message = FText::GetEmpty());
    bool EnsureDPSWidget();
    UMythicAffixRowWidget *AcquireAffixRow(int32 PoolIndex);
    void ClearAttackPresentation();
    void ClearItemStatSections();
    void ClearAffixPanels();
    void UpdateAffixPanelVisibility() const;
    void CancelPendingAffixRefresh();

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicItemInstance> RequestedCandidateItem;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicItemInstance> RequestedBaselineItem;

    FGuid RequestedCandidateGuid;
    FGuid RequestedBaselineGuid;
    FText RequestedTargetLabel;
    int32 RequestedTargetSlotIndex = INDEX_NONE;
    bool bRequestedComparisonActive = false;
    bool bRequestedTargetEmpty = true;
    bool bRequestedCanCycleTarget = false;

    /** Single pooled attack widget owned for the lifetime of this details card. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicDPSWidget> ActiveDPSWidget;

    /** High-water affix-row pool; rows are detached, cleared, and reused rather than destroyed per selection. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMythicAffixRowWidget>> AffixRowPool;

    int32 ActiveAffixRowCount = 0;

    /** Registry observed while an item is open, so editor publications cannot leave projected rows stale. */
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> BoundItemizationRegistry;

    FDelegateHandle SemanticDataChangedHandle;

    /** Consumer-side cancellation token for the registry's coalesced, one-shot readiness callbacks. */
    uint32 AffixRefreshSerial = 0;
};
