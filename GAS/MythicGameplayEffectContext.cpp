#include "MythicGameplayEffectContext.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serialization/GameplayEffectContextNetSerializer.h"
#endif

#if UE_WITH_IRIS
namespace UE::Net {
    // Forward to FGameplayEffectContextNetSerializer
    // Note: If FMythicGameplayEffectContext::NetSerialize() is modified, a custom NetSerializesr must be implemented as the current fallback will no longer be sufficient.
    UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(MythicGameplayEffectContext, FGameplayEffectContextNetSerializer);
}
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicGameplayEffectContext)

FMythicGameplayEffectContext *FMythicGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle) {
    FGameplayEffectContext *BaseEffectContext = Handle.Get();
    if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(StaticStruct())) {
        return static_cast<FMythicGameplayEffectContext *>(BaseEffectContext);
    }

    return nullptr;
}

void FMythicGameplayEffectContext::SetAbilitySource(const IMythicAbilitySourceInterface *InObject, float InSourceLevel) {
    AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
}

const IMythicAbilitySourceInterface *FMythicGameplayEffectContext::GetAbilitySource() const {
    return Cast<IMythicAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial *FMythicGameplayEffectContext::GetPhysicalMaterial() const {
    if (const FHitResult *HitResultPtr = GetHitResult()) {
        return HitResultPtr->PhysMaterial.Get();
    }
    return nullptr;
}

bool FMythicGameplayEffectContext::NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
    enum RepFlag {
        REP_Instigator,
        REP_EffectCauser,
        REP_AbilityCDO,
        REP_SourceObject,
        REP_Actors,
        REP_HitResult,
        REP_WorldOrigin,
        REP_IsCriticalHit,
        REP_IsBleed,
        REP_IsBurn,
        REP_IsPoison,
        REP_IsStun,
        REP_IsSlow,
        REP_IsWeaken,
        REP_IsFreeze,
        REP_IsTerrify,
        REP_MAX
    };
    uint32 RepBits = 0;
    if (Ar.IsSaving()) {
        if (bReplicateInstigator && Instigator.IsValid()) {
            RepBits |= 1 << REP_Instigator;
        }
        if (bReplicateEffectCauser && EffectCauser.IsValid()) {
            RepBits |= 1 << REP_EffectCauser;
        }
        if (AbilityCDO.IsValid()) {
            RepBits |= 1 << REP_AbilityCDO;
        }
        if (bReplicateSourceObject && SourceObject.IsValid()) {
            RepBits |= 1 << REP_SourceObject;
        }
        if (Actors.Num() > 0) {
            RepBits |= 1 << REP_Actors;
        }
        if (HitResult.IsValid()) {
            RepBits |= 1 << REP_HitResult;
        }
        if (bHasWorldOrigin) {
            RepBits |= 1 << REP_WorldOrigin;
        }
        if (bCriticalHit) {
            RepBits |= 1 << REP_IsCriticalHit;
        }
        if (bBleed) {
            RepBits |= 1 << REP_IsBleed;
        }
        if (bBurn) {
            RepBits |= 1 << REP_IsBurn;
        }
        if (bPoison) {
            RepBits |= 1 << REP_IsPoison;
        }
        if (bStun) {
            RepBits |= 1 << REP_IsStun;
        }
        if (bSlow) {
            RepBits |= 1 << REP_IsSlow;
        }
        if (bWeaken) {
            RepBits |= 1 << REP_IsWeaken;
        }
        if (bFreeze) {
            RepBits |= 1 << REP_IsFreeze;
        }
        if (bTerrify) {
            RepBits |= 1 << REP_IsTerrify;
        }
    }

    Ar.SerializeBits(&RepBits, REP_MAX);

    if (RepBits & (1 << REP_Instigator)) {
        Ar << Instigator;
    }
    if (RepBits & (1 << REP_EffectCauser)) {
        Ar << EffectCauser;
    }
    if (RepBits & (1 << REP_AbilityCDO)) {
        Ar << AbilityCDO;
    }
    if (RepBits & (1 << REP_SourceObject)) {
        Ar << SourceObject;
    }
    if (RepBits & (1 << REP_Actors)) {
        SafeNetSerializeTArray_Default<31>(Ar, Actors);
    }
    if (RepBits & (1 << REP_HitResult)) {
        if (Ar.IsLoading()) {
            if (!HitResult.IsValid()) {
                HitResult = MakeShared<FHitResult>();
            }
        }
        HitResult->NetSerialize(Ar, Map, bOutSuccess);
    }
    if (RepBits & (1 << REP_WorldOrigin)) {
        Ar << WorldOrigin;
        bHasWorldOrigin = true;
    }
    else {
        bHasWorldOrigin = false;
    }
    if (RepBits & (1 << REP_IsCriticalHit)) {
        Ar << bCriticalHit;
    }
    if (RepBits & (1 << REP_IsBleed)) {
        Ar << bBleed;
    }
    if (RepBits & (1 << REP_IsBurn)) {
        Ar << bBurn;
    }
    if (RepBits & (1 << REP_IsPoison)) {
        Ar << bPoison;
    }
    if (RepBits & (1 << REP_IsStun)) {
        Ar << bStun;
    }
    if (RepBits & (1 << REP_IsSlow)) {
        Ar << bSlow;
    }
    if (RepBits & (1 << REP_IsWeaken)) {
        Ar << bWeaken;
    }
    if (RepBits & (1 << REP_IsFreeze)) {
        Ar << bFreeze;
    }
    if (RepBits & (1 << REP_IsTerrify)) {
        Ar << bTerrify;
    }

    if (Ar.IsLoading()) {
        AddInstigator(Instigator.Get(), EffectCauser.Get()); // Just to initialize InstigatorAbilitySystemComponent
    }

    bOutSuccess = true;
    return true;
}
