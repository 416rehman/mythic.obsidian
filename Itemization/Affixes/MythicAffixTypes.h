#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Stats/MythicStatTypes.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "MythicAffixTypes.generated.h"

class UMythicAffixDefinition;
class UMythicAffixPool;
class UMythicAffixProfile;
class UMythicAffixRollPolicy;
class UPackageMap;

/** Typed authoring reference to one player-facing Affix Definition asset. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixDefinitionHandle {
    GENERATED_BODY()

    /** Exact Affix Definition selected by designers; no parallel name or identifier is authored. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix",
              meta = (AssetBundles = "Runtime", DisplayName = "Affix Definition"))
    TSoftObjectPtr<UMythicAffixDefinition> Asset;

    /** Returns Asset Manager identity derived from the typed reference without synchronously loading it. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the definition only when already resident; never synchronously loads. */
    UMythicAffixDefinition *GetAsset() const;

    /** Assigns the exact typed definition reference. */
    void SetAsset(UMythicAffixDefinition *InAsset);

    bool IsValid() const { return GetPrimaryAssetId().IsValid(); }
    void Reset() { Asset.Reset(); }
    bool operator==(const FMythicAffixDefinitionHandle &Other) const { return Asset == Other.Asset; }
    bool operator!=(const FMythicAffixDefinitionHandle &Other) const { return !(*this == Other); }
};

/** Typed authoring reference to one weighted Affix Pool asset. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixPoolHandle {
    GENERATED_BODY()

    /** Exact Affix Pool selected by designers; no parallel name or identifier is authored. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix",
              meta = (AssetBundles = "Runtime", DisplayName = "Affix Pool"))
    TSoftObjectPtr<UMythicAffixPool> Asset;

    /** Returns Asset Manager identity derived from the typed reference without synchronously loading it. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the pool only when already resident; never synchronously loads. */
    UMythicAffixPool *GetAsset() const;

    /** Assigns the exact typed pool reference. */
    void SetAsset(UMythicAffixPool *InAsset);

    bool IsValid() const { return GetPrimaryAssetId().IsValid(); }
    void Reset() { Asset.Reset(); }
    bool operator==(const FMythicAffixPoolHandle &Other) const { return Asset == Other.Asset; }
    bool operator!=(const FMythicAffixPoolHandle &Other) const { return !(*this == Other); }
};

/** Typed authoring reference to one Affix Roll Policy asset. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixRollPolicyHandle {
    GENERATED_BODY()

    /** Exact Affix Roll Policy selected by designers; no parallel name or identifier is authored. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix",
              meta = (AssetBundles = "Runtime", DisplayName = "Affix Roll Policy"))
    TSoftObjectPtr<UMythicAffixRollPolicy> Asset;

    /** Returns Asset Manager identity derived from the typed reference without synchronously loading it. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the policy only when already resident; never synchronously loads. */
    UMythicAffixRollPolicy *GetAsset() const;

    /** Assigns the exact typed policy reference. */
    void SetAsset(UMythicAffixRollPolicy *InAsset);

    bool IsValid() const { return GetPrimaryAssetId().IsValid(); }
    void Reset() { Asset.Reset(); }
    bool operator==(const FMythicAffixRollPolicyHandle &Other) const { return Asset == Other.Asset; }
    bool operator!=(const FMythicAffixRollPolicyHandle &Other) const { return !(*this == Other); }
};

/** Typed authoring reference to one concrete item-family Affix Profile asset. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixProfileHandle {
    GENERATED_BODY()

    /** Exact concrete Affix Profile selected by the item; signature items use their own profile asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix",
              meta = (AssetBundles = "Runtime", DisplayName = "Affix Profile"))
    TSoftObjectPtr<UMythicAffixProfile> Asset;

    /** Returns Asset Manager identity derived from the typed reference without synchronously loading it. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the profile only when already resident; never synchronously loads. */
    UMythicAffixProfile *GetAsset() const;

    /** Assigns the exact typed profile reference. */
    void SetAsset(UMythicAffixProfile *InAsset);

    bool IsValid() const { return GetPrimaryAssetId().IsValid(); }
    void Reset() { Asset.Reset(); }
    bool operator==(const FMythicAffixProfileHandle &Other) const { return Asset == Other.Asset; }
    bool operator!=(const FMythicAffixProfileHandle &Other) const { return !(*this == Other); }
};

FORCEINLINE uint32 GetTypeHash(const FMythicAffixDefinitionHandle &Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}
FORCEINLINE uint32 GetTypeHash(const FMythicAffixPoolHandle &Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}
FORCEINLINE uint32 GetTypeHash(const FMythicAffixRollPolicyHandle &Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}
FORCEINLINE uint32 GetTypeHash(const FMythicAffixProfileHandle &Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}

/** Rounding contract applied once when an affix magnitude is rolled. */
UENUM(BlueprintType)
enum class EMythicAffixQuantizationMode : uint8 {
    None,
    WholeNumber,
    Step
};

/**
 * Deterministic magnitude rounding shared by generation, save, network, UI, and gameplay.
 * Nearest-value ties are symmetric: positive and negative halves both round away from zero.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixQuantization {
    GENERATED_BODY()

    /** Chooses continuous, integer, or fixed-step values. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EMythicAffixQuantizationMode Mode = EMythicAffixQuantizationMode::None;

    /** Positive increment used only by Step mode; half-step ties round away from zero. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (EditCondition = "Mode == EMythicAffixQuantizationMode::Step", EditConditionHides,
                      ClampMin = "0.000001"))
    float Step = 1.0f;

    bool IsValid() const;
    float Apply(float Value) const;
};

/** Duplicate-resolution behavior for multiple copies of the same semantic affix. */
UENUM(BlueprintType)
enum class EMythicAffixStackingRule : uint8 {
    StackAll,
    UniquePerItem,
    HighestPerItem,
    HighestOverall
};

/** Item-level scaling model for a tier's magnitude range. */
UENUM(BlueprintType)
enum class EMythicAffixScaleMode : uint8 {
    None,
    Linear,
    Curve
};

namespace MythicAffix {
/** Exact GAS operations supported by the permanent stat-source ledger. */
FORCEINLINE bool IsSupportedModifierOp(const EGameplayModOp::Type ModifierOp) {
    switch (ModifierOp) {
    case EGameplayModOp::AddBase:
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::DivideAdditive:
    case EGameplayModOp::Override:
    case EGameplayModOp::MultiplyCompound:
    case EGameplayModOp::AddFinal:
        return true;
    default:
        return false;
    }
}

/** Multiplicative/divisive channels cannot materialize a zero magnitude. */
FORCEINLINE bool ModifierRequiresNonZeroMagnitude(const EGameplayModOp::Type ModifierOp) {
    return ModifierOp == EGameplayModOp::MultiplyAdditive
        || ModifierOp == EGameplayModOp::DivideAdditive
        || ModifierOp == EGameplayModOp::MultiplyCompound;
}
}

/** SHA-256-derived fingerprint used for deterministic content and tamper diagnostics. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicContentHash {
    GENERATED_BODY()

    /** First 64 bits of the content fingerprint. */
    UPROPERTY(SaveGame) uint64 Word0 = 0;

    /** Second 64 bits of the content fingerprint. */
    UPROPERTY(SaveGame) uint64 Word1 = 0;

    /** Third 64 bits of the content fingerprint. */
    UPROPERTY(SaveGame) uint64 Word2 = 0;

    /** Fourth 64 bits of the content fingerprint. */
    UPROPERTY(SaveGame) uint64 Word3 = 0;

    bool IsZero() const { return (Word0 | Word1 | Word2 | Word3) == 0; }
    bool operator==(const FMythicContentHash &Other) const {
        return Word0 == Other.Word0 && Word1 == Other.Word1 && Word2 == Other.Word2
            && Word3 == Other.Word3;
    }
    bool operator!=(const FMythicContentHash &Other) const { return !(*this == Other); }
};

/** Non-authoritative generation audit data; no field here is used to resolve affix gameplay semantics. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixProvenance {
    GENERATED_BODY()

    /** Profile recipe used for generation; audit only. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FPrimaryAssetId ProfileId;

    /** Roll policy used for generation; audit only. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FPrimaryAssetId PolicyId;

    /** Selected pool, or empty for a guaranteed grant; audit only. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FPrimaryAssetId PoolId;

    /** Roll group charged for this affix. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGameplayTag RollGroup;

    /** Semantic source kind such as random, implicit, crafted, gem, or socket. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGameplayTag SourceKind;

    /** Profile revision recorded for telemetry and diagnostics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 ProfileRevision = 0;

    /** Policy revision recorded for telemetry and diagnostics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 PolicyRevision = 0;

    /** Pool revision recorded for telemetry and diagnostics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 PoolRevision = 0;

    /** Pool-row revision recorded for telemetry and diagnostics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 PoolRowRevision = 0;

    /** Affix Definition revision recorded for telemetry; runtime always uses the current definition. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 DefinitionRevision = 0;

    /** Guaranteed-grant identity, or empty for a random pool row. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid OriginGrantGuid;

    /** Profile-slice identity, or empty for a guaranteed grant. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid OriginSliceGuid;

    /** Selected pool-row identity, or empty for a guaranteed grant. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid OriginPoolRowGuid;

    /** Socket identity when granted by a socketed item. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid OriginSocketGuid;

    /** Persistent item-instance identity that owns or supplied this affix. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid SourceItemGuid;

    /** Gameplay-content fingerprint captured when the numeric roll was generated. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FMythicContentHash GameplayContentHash;

    /** Item mutation generation used to fence stale asynchronous work. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 MutationRevision = 0;

    /** Item level used to select a progression tier and scale its range. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 GeneratedItemLevel = 1;

    /** Item rarity whose policy budget governed generation. */
    UPROPERTY(BlueprintReadOnly, SaveGame) TEnumAsByte<EItemRarity> GeneratedRarity = EItemRarity::Common;

    /** Deterministic RNG implementation version used for the roll. */
    UPROPERTY(BlueprintReadOnly, SaveGame) int32 AlgorithmVersion = 1;
};

/** One immutable numeric roll tied directly to one authoritative Affix Definition. */
USTRUCT(BlueprintType)
struct MYTHIC_API FRolledAffix {
    GENERATED_BODY()

    /** Deterministic instance identity used by replication, reconciliation, and analytics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FGuid RollGuid;

    /** Direct typed reference to the authoritative player-facing Affix Definition. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FMythicAffixDefinitionHandle AffixDefinition;

    /** One-based rank in the contextual tier progression selected at generation time. */
    UPROPERTY(BlueprintReadOnly, SaveGame, meta = (ClampMin = "1")) int32 TierRank = 0;

    /** The single rolled numeric value; range, target stat, operation, formatting, and stacking stay live in data assets. */
    UPROPERTY(BlueprintReadOnly, SaveGame) float Magnitude = 0.0f;

    /** Exact recipe and origin audit trail; never used to resolve gameplay semantics. */
    UPROPERTY(BlueprintReadOnly, SaveGame) FMythicAffixProvenance Provenance;

    /** Whether crafting is forbidden from rerolling or replacing this affix. */
    UPROPERTY(BlueprintReadOnly, SaveGame) bool bIsLocked = false;

    bool Serialize(FArchive &Ar);
    bool NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess);
    bool IsGameplayValid() const;
};

template <>
struct TStructOpsTypeTraits<FRolledAffix> : TStructOpsTypeTraitsBase2<FRolledAffix> {
    enum { WithSerializer = true, WithNetSerializer = true };
};

UINTERFACE()
class UMythicAffixSnapshotOwner : public UInterface {
    GENERATED_BODY()
};

/** Receives replicated affix-array changes after Fast Array reconciliation. */
class MYTHIC_API IMythicAffixSnapshotOwner {
    GENERATED_BODY()
public:
    virtual void OnAffixSnapshotsReplicated() = 0;
};

/** Fast Array row for one rolled affix. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedAffixItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Replicated and saved rolled affix payload. */
    UPROPERTY(SaveGame) FRolledAffix Affix;
};

/** Replicated collection of rolled affixes owned by an item fragment or socket. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedAffixArray : public FFastArraySerializer {
    GENERATED_BODY()

    /** Current rolled-affix rows. */
    UPROPERTY(SaveGame) TArray<FMythicReplicatedAffixItem> Items;

    /** Non-owning runtime callback target; deliberately outside reflection, replication, duplication, and persistence. */
    TWeakObjectPtr<UObject> Owner;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicReplicatedAffixItem,
            FMythicReplicatedAffixArray>(Items, DeltaParms, *this);
    }
    bool Serialize(FArchive &Ar);
    void PostReplicatedAdd(const TArrayView<int32> &Added, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &Changed, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> &Removed, int32 FinalSize);
    void SetOwner(UObject *InOwner) { Owner = InOwner; }
    void ReplaceAll(TArray<FRolledAffix> &&Snapshots);
    void GetSnapshots(TArray<FRolledAffix> &Out) const;
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedAffixArray>
    : TStructOpsTypeTraitsBase2<FMythicReplicatedAffixArray> {
    enum { WithNetDeltaSerializer = true, WithSerializer = true };
    static constexpr EPropertyObjectReferenceType WithSerializerObjectReferences =
        EPropertyObjectReferenceType::None;
};

namespace MythicAffixSerialization {
MYTHIC_API extern const FGuid RolledAffixMagic;
inline constexpr int32 RolledAffixVersion = 1;
inline constexpr int32 MaxAffixesPerContainer = 1024;
inline constexpr int32 MaxSoftPathCharacters = 2048;
}
