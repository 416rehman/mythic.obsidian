#pragma once

#include "CoreMinimal.h"

#include "MythicEntityId.generated.h"

class UMythicEntityPresentationRegistry;
class UPackageMap;

/** Authority-owned persistence domain for a canonical Mythic entity identity. */
UENUM(BlueprintType)
enum class EMythicEntityDomain : uint8 {
    Invalid = 0 UMETA(Hidden),
    LivingWorld,
    PlayerCharacter,
    AuthoredWorld,
    Runtime,
};

/**
 * Stable logical identity used by authority simulation, saves, and entitled owner-only knowledge.
 *
 * This value is deliberately opaque to Blueprint and must never be placed in public presentation replication.
 * Name seeds, actor names, row names, and strings are not entity identity and cannot construct this type.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityId {
    GENERATED_BODY()

public:
    FMythicEntityId() = default;

    /** Reconstructs a canonical identity at an authority persistence boundary; invalid input produces an invalid ID. */
    static FMythicEntityId FromAuthorityGuid(const EMythicEntityDomain InDomain,
                                             const FGuid &InValue) {
        FMythicEntityId Result;
        if (IsSupportedDomain(InDomain) && InValue.IsValid()) {
            Result.Domain = InDomain;
            Result.Value = InValue;
        }
        return Result;
    }

    /** Returns true only when both the typed domain and the 128-bit canonical value are valid. */
    bool IsValid() const {
        return IsSupportedDomain(Domain) && Value.IsValid();
    }

    /** Clears this value to the non-identifying invalid state. */
    void Reset() {
        Domain = EMythicEntityDomain::Invalid;
        Value.Invalidate();
    }

    /** Returns the authority persistence domain without converting it to a string or untyped key. */
    EMythicEntityDomain GetDomain() const { return Domain; }

    /** Returns the canonical GUID for authority persistence code; callers must not copy it into public state. */
    const FGuid &GetAuthorityGuid() const { return Value; }

    /** Produces a developer-only diagnostic representation; gameplay and persistence must use the typed value. */
    FString ToDebugString() const {
        if (!IsValid()) {
            return TEXT("EntityId[Invalid]");
        }

        return FString::Printf(TEXT("EntityId[%u:%s]"),
                               static_cast<uint8>(Domain),
                               *Value.ToString(EGuidFormats::DigitsWithHyphens));
    }

    /** Serializes an explicit validity bit, typed domain, and GUID; malformed input is cleared and rejected. */
    bool NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
        (void)Map;

        uint8 bHasIdentity = IsValid() ? 1u : 0u;
        Ar.SerializeBits(&bHasIdentity, 1);

        if (Ar.IsLoading() && bHasIdentity == 0u) {
            Reset();
            bOutSuccess = !Ar.IsError();
            return true;
        }

        if (bHasIdentity == 0u) {
            bOutSuccess = !Ar.IsError();
            return true;
        }

        uint8 SerializedDomain = static_cast<uint8>(Domain);
        Ar.SerializeBits(&SerializedDomain, 3);

        FGuid SerializedValue = Value;
        Ar << SerializedValue.A;
        Ar << SerializedValue.B;
        Ar << SerializedValue.C;
        Ar << SerializedValue.D;

        if (Ar.IsLoading()) {
            const EMythicEntityDomain LoadedDomain =
                static_cast<EMythicEntityDomain>(SerializedDomain);
            if (!IsSupportedDomain(LoadedDomain) || !SerializedValue.IsValid()) {
                Reset();
                bOutSuccess = false;
                return true;
            }

            Domain = LoadedDomain;
            Value = SerializedValue;
        }

        bOutSuccess = !Ar.IsError();
        return true;
    }

    bool operator==(const FMythicEntityId &Other) const {
        return Domain == Other.Domain && Value == Other.Value;
    }

    bool operator!=(const FMythicEntityId &Other) const {
        return !(*this == Other);
    }

private:
    static bool IsSupportedDomain(const EMythicEntityDomain Candidate) {
        return Candidate >= EMythicEntityDomain::LivingWorld
               && Candidate <= EMythicEntityDomain::Runtime;
    }

    // SaveGame properties are intentionally not Blueprint-visible: canonical identity is authority/private data.
    UPROPERTY(SaveGame)
    EMythicEntityDomain Domain = EMythicEntityDomain::Invalid;

    UPROPERTY(SaveGame)
    FGuid Value;

    friend uint32 GetTypeHash(const FMythicEntityId &Id);
};

FORCEINLINE uint32 GetTypeHash(const FMythicEntityId &Id) {
    uint32 Hash = ::GetTypeHash(static_cast<uint8>(Id.Domain));
    Hash = HashCombineFast(Hash, ::GetTypeHash(Id.Value.A));
    Hash = HashCombineFast(Hash, ::GetTypeHash(Id.Value.B));
    Hash = HashCombineFast(Hash, ::GetTypeHash(Id.Value.C));
    return HashCombineFast(Hash, ::GetTypeHash(Id.Value.D));
}

template <>
struct TStructOpsTypeTraits<FMythicEntityId>
    : TStructOpsTypeTraitsBase2<FMythicEntityId> {
    enum {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};

/**
 * Opaque public nonce for one presentation embodiment.
 *
 * It is freshly authority-generated for every bind or presentation-identity boundary, is never saved, and cannot be
 * used to correlate separate embodiments. Consumers must pair it with the embodiment generation.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPresentationHandle {
    GENERATED_BODY()

public:
    FMythicPresentationHandle() = default;

    /** Returns true when this handle contains a nonzero opaque embodiment nonce. */
    bool IsValid() const { return Value.IsValid(); }

    /** Clears this public handle so it cannot resolve an embodiment. */
    void Reset() { Value.Invalidate(); }

    /** Produces a developer-only diagnostic token; it is not a persistence or player-knowledge key. */
    FString ToDebugString() const {
        return IsValid()
                   ? FString::Printf(TEXT("Presentation[%s]"),
                                     *Value.ToString(EGuidFormats::DigitsWithHyphens))
                   : TEXT("Presentation[Invalid]");
    }

    /** Serializes an explicit validity bit and opaque nonce; an invalid nonce always deserializes as cleared. */
    bool NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
        (void)Map;

        uint8 bHasHandle = IsValid() ? 1u : 0u;
        Ar.SerializeBits(&bHasHandle, 1);

        if (Ar.IsLoading() && bHasHandle == 0u) {
            Reset();
            bOutSuccess = !Ar.IsError();
            return true;
        }

        if (bHasHandle == 0u) {
            bOutSuccess = !Ar.IsError();
            return true;
        }

        FGuid SerializedValue = Value;
        Ar << SerializedValue.A;
        Ar << SerializedValue.B;
        Ar << SerializedValue.C;
        Ar << SerializedValue.D;

        if (Ar.IsLoading()) {
            if (!SerializedValue.IsValid()) {
                Reset();
                bOutSuccess = false;
                return true;
            }
            Value = SerializedValue;
        }

        bOutSuccess = !Ar.IsError();
        return true;
    }

    bool operator==(const FMythicPresentationHandle &Other) const {
        return Value == Other.Value;
    }

    bool operator!=(const FMythicPresentationHandle &Other) const {
        return !(*this == Other);
    }

private:
    static FMythicPresentationHandle FromAuthorityNonce(const FGuid &Nonce) {
        FMythicPresentationHandle Result;
        if (Nonce.IsValid()) {
            Result.Value = Nonce;
        }
        return Result;
    }

    // The nonce is intentionally opaque to Blueprint; only the authority registry can construct it.
    UPROPERTY()
    FGuid Value;

    friend class UMythicEntityPresentationRegistry;
    friend uint32 GetTypeHash(const FMythicPresentationHandle &Handle);
};

FORCEINLINE uint32 GetTypeHash(const FMythicPresentationHandle &Handle) {
    uint32 Hash = ::GetTypeHash(Handle.Value.A);
    Hash = HashCombineFast(Hash, ::GetTypeHash(Handle.Value.B));
    Hash = HashCombineFast(Hash, ::GetTypeHash(Handle.Value.C));
    return HashCombineFast(Hash, ::GetTypeHash(Handle.Value.D));
}

template <>
struct TStructOpsTypeTraits<FMythicPresentationHandle>
    : TStructOpsTypeTraitsBase2<FMythicPresentationHandle> {
    enum {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};
