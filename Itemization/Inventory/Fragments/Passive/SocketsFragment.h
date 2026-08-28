#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "SocketsFragment.generated.h"

class UMythicAffixApplicationComponent;
class UMythicItemizationDataRegistrySubsystem;

/**
 * Host-owned socket state.
 *
 * SocketStates is the sole runtime/save authority. Socketed affixes retain typed Affix Definition references and
 * one numeric roll; live definitions supply their current stat and operation semantics.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API USocketsFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Sockets)

    UPROPERTY(Replicated, SaveGame)
    FMythicReplicatedSocketArray SocketStates;

    /** Per-item socket-count policy override. Empty uses the code-default table. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
    FMythicSocketCountTable CountTableOverride;

    /** Optional colour assigned to every newly rolled socket; empty means universal. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets",
              meta = (Categories = "Itemization.Gem"))
    FGameplayTag RolledSocketColor;

    /** Returns the authoritative number of sockets currently owned by the host item. */
    UFUNCTION(BlueprintPure, Category = "Sockets")
    int32 GetSocketCount() const;

    /** Returns how many authoritative host sockets currently contain a gem. */
    UFUNCTION(BlueprintPure, Category = "Sockets")
    int32 GetFilledSocketCount() const;

    const FMythicReplicatedSocketItem *GetSocketState(int32 SocketIndex) const;
    FGameplayTag GetSocketColor(int32 SocketIndex) const;
    FGameplayTag GetSocketedGemType(int32 SocketIndex) const;
    bool IsSocketFilled(int32 SocketIndex) const;
    /** True once every socketed roll resolves through the resident typed Affix Definition closure. */
    bool IsRuntimeDataReady() const { return bRuntimeDataReady; }

    /**
     * Canonical authority-only insertion. Source snapshots are copied, host-rekeyed, and committed only after the
     * equipped permanent-stat ledger accepts the complete transaction.
     */
    bool ServerSocketGem(int32 SocketIndex, const FGameplayTag &GemType, const FGuid &SourceGemItemGuid,
                         TConstArrayView<FRolledAffix> GemAffixSnapshots);

    FGameplayTag ServerUnsocketGem(int32 SocketIndex);
    bool ServerAddSocket();

    /** Idempotent async typed-definition closure request for restored or replicated socket snapshots. */
    void RequestRuntimeData();

    virtual void Serialize(FArchive &Ar) override;
    virtual void PostLoad() override;
    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Fast Array callback target; restores ownership and coalesces owning-slot presentation. */
    void OnSocketStatesReplicated();

    /** Derives a stable host-local roll identity from physical item, socket, gem, and source-roll identities. */
    static FGuid DeriveSocketAffixRollGuid(const FGuid &HostItemGuid, const FGuid &SocketGuid,
                                           const FGuid &SourceGemItemGuid, const FGuid &SourceRollGuid);

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif

private:
    bool HasServerAuthority() const;
    UMythicItemizationDataRegistrySubsystem *ResolveRegistry() const;
    UMythicAffixApplicationComponent *ResolveApplicationComponent(UMythicItemInstance *ItemInstance) const;
    bool BuildSocketCandidates(const FGuid &SocketGuid, const FGuid &SourceGemItemGuid,
                               TConstArrayView<FRolledAffix> GemSnapshots,
                               TArray<FRolledAffix> &OutCandidates) const;
    bool HasHostRollGuidCollision(const TSet<FGuid> &CandidateRollGuids) const;
    bool ApplySnapshotsCommitLast(TConstArrayView<FRolledAffix> Snapshots,
                                  UMythicAffixApplicationComponent &Application);
    bool RemoveSnapshotsCommitLast(TConstArrayView<FRolledAffix> Snapshots,
                                   UMythicAffixApplicationComponent &Application);
    void RequestCanonicalSnapshotClosure(uint32 RequestRevision);
    void HandleCanonicalClosureReady(bool bSuccess, uint32 RequestRevision);
    void RefreshOwningInventoryPresentation() const;

    /** Cached permanent-stat application component used while the host item is equipped. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicAffixApplicationComponent> ActiveApplicationComponent = nullptr;

    /** True while the host item is equipped and socket mutations must reconcile permanent stats transactionally. */
    UPROPERTY(Transient)
    bool bItemActive = false;

    /** True while the typed definition closure for restored or replicated snapshots is loading. */
    UPROPERTY(Transient)
    bool bRuntimeRequestInFlight = false;

    /** True after invalid socket state or closure failure quarantines this fragment. */
    UPROPERTY(Transient)
    bool bRuntimeDataFailed = false;

    /** True once every socket snapshot has a resident typed Affix Definition. */
    UPROPERTY(Transient)
    bool bRuntimeDataReady = false;

    /** Monotonic fence that prevents stale asynchronous closures from publishing readiness. */
    uint32 RuntimeDataRevision = 0;
};
