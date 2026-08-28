#pragma once

#include "CoreMinimal.h"
#include "Itemization/Affixes/MythicAffixGeneration.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "AffixesFragment.generated.h"

class UMythicItemizationDataRegistrySubsystem;

/** Small authoritative item-affix state that is independent of the immutable rolled snapshots. */
USTRUCT(BlueprintType)
struct MYTHIC_API FAffixesRuntimeReplicatedData {
    GENERATED_BODY()

    /** True when current affix state failed validation and authoritative crafting must remain disabled. */
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bCorrupted = false;
};

/** Immutable item-template reference to the one canonical affix profile used for generation. */
USTRUCT(BlueprintType)
struct MYTHIC_API FAffixesConfig {
    GENERATED_BODY()
    /** Canonical profile whose policy, guaranteed grants and closed pools define this item's affix generation. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FMythicAffixProfileHandle AffixProfile;
};

/** Instanced item fragment that owns immutable affix snapshots, corruption state, and authoritative crafting seams. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UAffixesFragment : public UItemFragment, public IMythicAffixSnapshotOwner {
    GENERATED_BODY()
public:
    DECLARE_FRAGMENT(Affixes)

    /** Current item-template configuration. Replicated, never persisted. */
    UPROPERTY(ReplicatedUsing = OnRep_AffixesConfig, EditDefaultsOnly, BlueprintReadOnly,
              meta = (ShowOnlyInnerProperties))
    FAffixesConfig AffixesConfig;

    /** Immutable gameplay snapshots, owner replicated through Fast Array and SaveGame serialized. */
    UPROPERTY(Replicated, SaveGame)
    FMythicReplicatedAffixArray AffixSnapshots;

    /** Authoritative corruption state kept separate from the immutable rolled-affix collection. */
    UPROPERTY(Replicated, SaveGame)
    FAffixesRuntimeReplicatedData AffixesRuntimeReplicatedData;

    virtual void Serialize(FArchive &Ar) override;
    virtual void PostLoad() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    virtual void OnAffixSnapshotsReplicated() override;

    /** Internal authority-only crafting operation invoked through the validated player control layer. */
    bool RerollUnlockedAffixes(int32 ItemLevel);
    /** Internal authority-only lock mutation invoked through the validated player control layer. */
    void SetAffixLocked(int32 AffixIndex, bool bLocked);
    /** Reports whether validation quarantined this item's affix state from authoritative crafting or application. */
    UFUNCTION(BlueprintPure, Category = "Affixes") bool IsCorrupted() const { return AffixesRuntimeReplicatedData.bCorrupted; }
    /** Returns whether an authoritative crafting operation is currently legal and supplies a player-readable refusal reason. */
    UFUNCTION(BlueprintPure, Category = "Affixes") bool CanApplyCraftOp(FText &OutReason) const;
    /** Internal authority-only irreversible crafting result that marks the item's affix state corrupted. */
    void ServerCorruptItem();
    /** Refreshes item presentation after the replicated profile configuration arrives. */
    UFUNCTION() void OnRep_AffixesConfig();

    const FMythicReplicatedAffixArray &GetAffixSnapshots() const { return AffixSnapshots; }

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif

private:
    bool HasServerAuthority() const;
    UMythicItemizationDataRegistrySubsystem *ResolveRegistry() const;
    void RefreshOwningInventoryPresentation(bool bReconcileAffixes = true) const;
    bool bGenerationFailed = false;
};
