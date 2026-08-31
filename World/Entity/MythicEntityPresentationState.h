#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicEntityPresentationState.generated.h"

class UMythicEntityPresentationComponent;

/**
 * Quantized public vitality for one exact visible embodiment.
 *
 * Authority derives this from canonical health attributes. It intentionally carries only a normalized fraction, so
 * clients cannot recover exact health, maximum health, scaling coefficients, or any private combat-state inputs.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPublicVitalitySnapshot {
    GENERATED_BODY()

    /** Exact opaque embodiment owning this snapshot; a generation mismatch makes the value unusable. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation|Vitality")
    FMythicEntityPresentationInstance Instance;

    /** True only when authority resolved finite health against a positive finite maximum for this embodiment. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation|Vitality")
    bool bValid = false;

    /** Normalized health quantized to 256 transport values; use GetHealthFraction for presentation math. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation|Vitality",
              meta = (ClampMin = "0", ClampMax = "255"))
    uint8 HealthFractionQuantized = 0;

    /** Nonzero authority revision used to reject delayed work within the current embodiment. */
    UPROPERTY()
    uint32 Revision = 0;

    /** Returns true only when this value belongs to the supplied exact active embodiment. */
    bool IsCurrentFor(const FMythicEntityPresentationInstance &ExpectedInstance) const {
        return bValid && Revision != 0 && Instance == ExpectedInstance;
    }

    /** Clears the snapshot so pooled actors cannot retain vitality from their previous occupant. */
    void Reset() {
        Instance.Reset();
        bValid = false;
        HealthFractionQuantized = 0;
        Revision = 0;
    }
};

/** One authority-owned, stateful public fact for the current visible embodiment. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicObservableFactItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Opaque public subject handle; it must match the owning component's current active snapshot. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FMythicPresentationHandle Subject;

    /** Current nonzero embodiment generation; stale pooled-actor rows are rejected when this differs. */
    UPROPERTY()
    uint32 EmbodimentGeneration = 0;

    /** Semantic single-writer slot such as LivingWorld.Observable.Activity; one slot owns one current value. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FGameplayTag FactSlotTag;

    /** Safe publicly observable value; this represents executed state, never private intent or UI state. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FGameplayTag ValueTag;

    /** Optional currently visible related embodiment; invalid means the fact has no public related subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FMythicPresentationHandle RelatedSubject;

    /** Nonzero authority revision used to reject delayed work and order changes inside this embodiment. */
    UPROPERTY()
    uint32 Revision = 0;
};

/** Delta-replicated public observable facts for one presentation component. */
USTRUCT()
struct MYTHIC_API FMythicObservableFactArray : public FFastArraySerializer {
    GENERATED_BODY()

    /** Associates client replication callbacks with this array's non-shareable component owner. */
    void SetOwner(UMythicEntityPresentationComponent *InOwner) { Owner = InOwner; }

    /** Read-only current fact rows; authority mutation is available only through the presentation component. */
    const TArray<FMythicObservableFactItem> &GetItems() const { return Items; }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FMythicObservableFactItem, FMythicObservableFactArray>(Items, DeltaParms, *this);
    }

    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters &Parameters);

private:
    friend class UMythicEntityPresentationComponent;

    UPROPERTY()
    TArray<FMythicObservableFactItem> Items;

    UMythicEntityPresentationComponent *Owner = nullptr;
};

template <>
struct TStructOpsTypeTraits<FMythicObservableFactArray> : TStructOpsTypeTraitsBase2<FMythicObservableFactArray> {
    enum { WithNetDeltaSerializer = true };
};

/** Bounded public rendering data for one canonical world-visible active status. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPublicStatusPresentationItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Opaque public subject handle; it must match the owning component's current active snapshot. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FMythicPresentationHandle Subject;

    /** Current nonzero embodiment generation; stale Minimal-GAS status rows are rejected when this differs. */
    UPROPERTY()
    uint32 EmbodimentGeneration = 0;

    /** Canonical Status.Type.* key resolved through UMythicStatusRegistry; effect classes never cross this boundary. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation", meta = (Categories = "Status.Type"))
    FGameplayTag StatusType;

    /** Server-world-time deadline in seconds; zero means duration is unknown or intentionally not presented. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    double ServerEndTimeSeconds = 0.0;

    /** Bounded public stack count in [0,255]; zero means the definition hides stacks or no count is available. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    uint8 StackCount = 0;

    /** Bounded authored/gameplay severity in [0,255]; zero means no severity variant is presented. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    uint8 Severity = 0;

    /** Nonzero authority revision used to reject delayed work and order changes inside this embodiment. */
    UPROPERTY()
    uint32 Revision = 0;
};

/** Delta-replicated, late-join-safe public statuses for one presentation component. */
USTRUCT()
struct MYTHIC_API FMythicPublicStatusPresentationArray : public FFastArraySerializer {
    GENERATED_BODY()

    /** Associates client replication callbacks with this array's non-shareable component owner. */
    void SetOwner(UMythicEntityPresentationComponent *InOwner) { Owner = InOwner; }

    /** Read-only status rows; authority mutation is available only through the canonical GAS/status adapter. */
    const TArray<FMythicPublicStatusPresentationItem> &GetItems() const { return Items; }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FMythicPublicStatusPresentationItem, FMythicPublicStatusPresentationArray>(Items, DeltaParms, *this);
    }

    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters &Parameters);

private:
    friend class UMythicEntityPresentationComponent;

    UPROPERTY()
    TArray<FMythicPublicStatusPresentationItem> Items;

    UMythicEntityPresentationComponent *Owner = nullptr;
};

template <>
struct TStructOpsTypeTraits<FMythicPublicStatusPresentationArray> : TStructOpsTypeTraitsBase2<FMythicPublicStatusPresentationArray> {
    enum { WithNetDeltaSerializer = true };
};
