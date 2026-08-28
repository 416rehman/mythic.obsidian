// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicItemDetailsWidget.generated.h"

class UMythicAffixRowWidget;
class UMythicDPSWidget;
class UMythicItemInstance;
class UMythicItemizationDataRegistrySubsystem;
class UNamedSlot;
class UPanelWidget;

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
     * Atomically rebuilds the dedicated weapon attack block and ordinary affix/stat rows, waiting asynchronously for
     * core semantic data when necessary. Passing null clears every item-stat presentation without destroying pools.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details",
              meta = (DisplayName = "Refresh Item Stat Sections"))
    void RefreshAffixSection(UMythicItemInstance *Item);

    /** Current readiness and rendering state of the affix section. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Item Details")
    EMythicAffixSectionState AffixSectionState = EMythicAffixSectionState::Empty;

protected:
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

private:
    void BindItemizationRegistry(UMythicItemizationDataRegistrySubsystem *Registry);
    void UnbindItemizationRegistry();
    void HandleSemanticDataChanged(uint64 SemanticRevision);
    void RebuildAffixRows(UMythicItemInstance *Item,
                          UMythicItemizationDataRegistrySubsystem *Registry,
                          uint32 RequestSerial);
    void SetAffixSectionState(EMythicAffixSectionState NewState, const FText &Message = FText::GetEmpty());
    bool EnsureDPSWidget();
    void ClearAttackPresentation();
    void ClearItemStatSections();
    void ClearAffixPanels() const;
    void UpdateAffixPanelVisibility() const;
    void CancelPendingAffixRefresh();

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicItemInstance> RequestedItem;

    /** Single pooled attack widget owned for the lifetime of this details card. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicDPSWidget> ActiveDPSWidget;

    /** Registry observed while an item is open, so editor publications cannot leave projected rows stale. */
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> BoundItemizationRegistry;

    FDelegateHandle SemanticDataChangedHandle;

    /** Consumer-side cancellation token for the registry's coalesced, one-shot readiness callbacks. */
    uint32 AffixRefreshSerial = 0;
};
