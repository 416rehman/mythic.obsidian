#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "MythicGemFragment.generated.h"

class UMythicItemizationDataRegistrySubsystem;

/**
 * Data-authored gem payload.
 *
 * GrantSpecs is the sole authoring route. Authority materializes those exact Affix Definition/tier-rank selections
 * into immutable, locked snapshots after the typed data closure is ready.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UMythicGemFragment : public UItemFragment, public IMythicAffixSnapshotOwner {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(MythicGem)

    /** Identity, socket compatibility colour and runeword element. */
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "Gem",
              meta = (Categories = "Itemization.Gem"))
    FGameplayTag GemType;

    /** Exact Affix Definition and tier-rank selections; no raw stats, ranges, or host scaling are authored here. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gem|Affixes")
    TArray<FMythicAffixGrantSpec> GrantSpecs;

    /** Current immutable runtime/save state. */
    UPROPERTY(Replicated, SaveGame)
    FMythicReplicatedAffixArray GrantedAffixSnapshots;

    /** Returns true for this canonical gem fragment; useful when presenting heterogeneous item fragments in Blueprint. */
    UFUNCTION(BlueprintPure, Category = "Gem")
    bool IsGem() const;

    /** Returns the authored gem identity used for socket compatibility and runeword sequence matching. */
    UFUNCTION(BlueprintPure, Category = "Gem")
    FGameplayTag GetGemType() const { return GemType; }

    /** Returns the materialized snapshot count, or the authored grant count before authority materializes a new gem. */
    UFUNCTION(BlueprintPure, Category = "Gem|Affixes")
    int32 GetGrantedAffixCount() const;

    /** Copies the current immutable snapshots for socket insertion/presentation. */
    void GetGrantedAffixSnapshots(TArray<FRolledAffix> &OutSnapshots) const;

    /** Idempotently starts the exact typed data closure. It never synchronously loads gameplay assets. */
    void RequestRuntimeData();

    /**
     * Factory staging seam for fresh GrantSpecs. Requires the exact typed grant closure to be ready, mutates only
     * this temporary fragment, and commits immutable snapshots only after every grant succeeds.
     */
    bool MaterializeFreshGrantsForItem(
        UMythicItemInstance &OwningItem,
        const UMythicItemizationDataRegistrySubsystem &ReadyRegistry);

    virtual void Serialize(FArchive &Ar) override;
    virtual void PostLoad() override;
    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    virtual void OnAffixSnapshotsReplicated() override;

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif

private:
    bool HasServerAuthority() const;
    UMythicItemizationDataRegistrySubsystem *ResolveRegistry() const;
    bool MaterializeFreshGrants();
    void HandleRuntimeDataReady(bool bSuccess, uint32 RequestRevision);
    void RefreshOwningInventoryPresentation() const;
    static uint64 DeriveGrantSeed(const FGuid &ItemInstanceGuid);
    /** True while the asynchronous typed grant closure is loading. */
    UPROPERTY(Transient)
    bool bRuntimeRequestInFlight = false;

    /** True after grant materialization fails closed; prevents repeated mutation attempts. */
    UPROPERTY(Transient)
    bool bRuntimeMaterializationFailed = false;

    /** Monotonic fence that prevents stale asynchronous closures from publishing or materializing data. */
    uint32 RuntimeDataRevision = 0;
};
