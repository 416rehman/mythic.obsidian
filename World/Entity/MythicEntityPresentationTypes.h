#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/WeakObjectPtr.h"
#include "World/Entity/MythicEntityId.h"

#include "MythicEntityPresentationTypes.generated.h"

class UMythicEntityPresentationComponent;
class UPackageMap;

/** Public, embodiment-scoped identity used to reject stale network and asynchronous presentation work. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityPresentationInstance {
    GENERATED_BODY()

    /** Opaque public nonce for the currently presented embodiment; invalid means there is no resolvable subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FMythicPresentationHandle Handle;

    /** Authority world-registry generation for this embodiment; zero is invalid and the typed pair stays opaque to Blueprint. */
    UPROPERTY()
    uint32 EmbodimentGeneration = 0;

    FMythicEntityPresentationInstance() = default;

    FMythicEntityPresentationInstance(const FMythicPresentationHandle &InHandle,
                                      const uint32 InEmbodimentGeneration)
        : Handle(InHandle), EmbodimentGeneration(InEmbodimentGeneration) {}

    /** Returns true only when both the opaque handle and nonzero authority generation are present. */
    bool IsValid() const {
        return Handle.IsValid() && EmbodimentGeneration != 0;
    }

    /** Clears both parts so stale work can no longer match this instance. */
    void Reset() {
        Handle.Reset();
        EmbodimentGeneration = 0;
    }

    /** Produces a developer-only diagnostic representation of this nonpersistent public instance. */
    FString ToDebugString() const {
        return IsValid()
                   ? FString::Printf(TEXT("%s@%u"), *Handle.ToDebugString(),
                                     EmbodimentGeneration)
                   : TEXT("PresentationInstance[Invalid]");
    }

    /** Serializes a clearable handle-generation pair and rejects malformed or partial network values. */
    bool NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
        uint8 bHasInstance = IsValid() ? 1u : 0u;
        Ar.SerializeBits(&bHasInstance, 1);

        if (Ar.IsLoading() && bHasInstance == 0u) {
            Reset();
            bOutSuccess = !Ar.IsError();
            return true;
        }

        if (bHasInstance == 0u) {
            bOutSuccess = !Ar.IsError();
            return true;
        }

        bool bHandleSuccess = false;
        Handle.NetSerialize(Ar, Map, bHandleSuccess);
        Ar.SerializeIntPacked(EmbodimentGeneration);

        if (Ar.IsLoading()
            && (!bHandleSuccess || !Handle.IsValid()
                || EmbodimentGeneration == 0)) {
            Reset();
            bOutSuccess = false;
            return true;
        }

        bOutSuccess = bHandleSuccess && !Ar.IsError();
        return true;
    }

    bool operator==(const FMythicEntityPresentationInstance &Other) const {
        return Handle == Other.Handle
               && EmbodimentGeneration == Other.EmbodimentGeneration;
    }

    bool operator!=(const FMythicEntityPresentationInstance &Other) const {
        return !(*this == Other);
    }
};

FORCEINLINE uint32 GetTypeHash(const FMythicEntityPresentationInstance &Instance) {
    return HashCombineFast(::GetTypeHash(Instance.Handle),
                           ::GetTypeHash(Instance.EmbodimentGeneration));
}

template <>
struct TStructOpsTypeTraits<FMythicEntityPresentationInstance>
    : TStructOpsTypeTraitsBase2<FMythicEntityPresentationInstance> {
    enum {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};

/**
 * Complete safe identity state replicated with an embodied actor.
 *
 * This snapshot contains no canonical entity ID, name seed, true faction, private role, or player-relative state.
 * An inactive snapshot must not be registered or used as an action target.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPublicIdentitySnapshot {
    GENERATED_BODY()

    /** Opaque public handle and generation for the active embodiment; invalid while the actor is pooled or unbound. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FMythicEntityPresentationInstance Instance;

    /** True only after authority binding is complete and the actor is safe to present; false clears client selection. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    bool bActive = false;

    /** Coarse externally observable kind such as humanoid, animal, construct, or object; never a hidden classification. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FGameplayTag PublicKindTag;

    /**
     * Optional typed key for the single authored public identity/cover definition. Role, species, public name, and
     * visibly presented faction are resolved only from this definition so private simulation tags cannot drift into
     * replication through parallel fields.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Presentation")
    FPrimaryAssetId PublicIdentityDefinitionId;

    /** Returns true only for a fully bound, publicly resolvable embodiment. */
    bool IsActive() const { return bActive && Instance.IsValid(); }

    /** Clears all public state so a pooled actor cannot retain presentation data from its prior occupant. */
    void Reset() {
        Instance.Reset();
        bActive = false;
        PublicKindTag = FGameplayTag();
        PublicIdentityDefinitionId = FPrimaryAssetId();
    }
};

/** Local non-owning resolution result; it is never reflected, replicated, or persisted. */
struct MYTHIC_API FMythicEntityInstanceHandle {
    FMythicEntityPresentationInstance Instance;
    TWeakObjectPtr<UMythicEntityPresentationComponent> Component;

    FMythicEntityInstanceHandle() = default;

    FMythicEntityInstanceHandle(
        const FMythicEntityPresentationInstance &InInstance,
        UMythicEntityPresentationComponent *InComponent);

    /** Returns true only while the component is alive and still represents this public instance. */
    bool IsValid() const;

    /** Clears both the public key and weak runtime target. */
    void Reset() {
        Instance.Reset();
        Component.Reset();
    }
};
