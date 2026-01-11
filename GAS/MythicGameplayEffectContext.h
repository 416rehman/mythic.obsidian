// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "MythicAbilitySourceInterface.h"

#include "MythicGameplayEffectContext.generated.h"

class AActor;
class FArchive;
class UObject;
class UPhysicalMaterial;

// #define that creates bool UPROPERTY, getter, setter
#define MYTHIC_CONTEXT_BOOL_PROPERTY(PropertyName) \
    protected: \
    UPROPERTY() \
    bool b##PropertyName = false; \
    public: \
    bool Is##PropertyName() const { return b##PropertyName; } \
    void Set##PropertyName(bool bIn##PropertyName) { b##PropertyName = bIn##PropertyName; }

USTRUCT(BlueprintType, Blueprintable)
struct FMythicGameplayEffectContext : public FGameplayEffectContext {
    GENERATED_BODY()

    FMythicGameplayEffectContext()
        : FGameplayEffectContext() {}

    FMythicGameplayEffectContext(AActor *InInstigator, AActor *InEffectCauser)
        : FGameplayEffectContext(InInstigator, InEffectCauser) {}

    /** Returns the wrapped FMythicGameplayEffectContext from the handle, or nullptr if it doesn't exist or is the wrong type */
    static FMythicGameplayEffectContext *ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

    /** Sets the object used as the ability source */
    void SetAbilitySource(const IMythicAbilitySourceInterface *InObject, float InSourceLevel);

    /** Returns the ability source interface associated with the source object. Only valid on the authority. */
    const IMythicAbilitySourceInterface *GetAbilitySource() const;
    const UPhysicalMaterial *GetPhysicalMaterial() const;

    // Is this a critical hit?
    MYTHIC_CONTEXT_BOOL_PROPERTY(CriticalHit)

    // Will/Did this bleed the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Bleed)

    // Will/Did this burn the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Burn)

    // Will/Did this poison the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Poison)

    // Will/Did this STUN the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Stun)

    // Will/Did this SLOW the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Slow)

    // Will/Did this WEAKEN the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Weaken)

    // Will/Did this FREEZE the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Freeze)

    // Will/Did this TERRIFY the target?
    MYTHIC_CONTEXT_BOOL_PROPERTY(Terrify)

    virtual FGameplayEffectContext *Duplicate() const override {
        FMythicGameplayEffectContext *NewContext = new FMythicGameplayEffectContext();
        *NewContext = *this;
        if (GetHitResult()) {
            // Does a deep copy of the hit result
            NewContext->AddHitResult(*GetHitResult(), true);
        }
        return NewContext;
    }

    virtual UScriptStruct *GetScriptStruct() const override {
        return StaticStruct();
    }

    /** Overridden to serialize new fields */
    virtual bool NetSerialize(FArchive &Ar, class UPackageMap *Map, bool &bOutSuccess) override;

protected:
    /** Ability Source object (should implement IMythicAbilitySourceInterface). NOT replicated currently */
    UPROPERTY()
    TWeakObjectPtr<const UObject> AbilitySourceObject;
};

template <>
struct TStructOpsTypeTraits<FMythicGameplayEffectContext> : TStructOpsTypeTraitsBase2<FMythicGameplayEffectContext> {
    enum {
        WithNetSerializer = true,
        WithCopy = true
    };
};

UCLASS()
class MYTHIC_API UMythicGameplayEffectContextLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetCriticalHit(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsCriticalHit) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetCriticalHit(bInIsCriticalHit);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetBleed(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsBleed) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetBleed(bInIsBleed);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetBurn(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsBurn) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetBurn(bInIsBurn);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetPoison(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsPoison) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetPoison(bInIsPoison);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetStun(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsStun) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetStun(bInIsStun);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetSlow(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsSlow) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetSlow(bInIsSlow);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetWeaken(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsWeaken) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetWeaken(bInIsWeaken);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetFreeze(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsFreeze) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetFreeze(bInIsFreeze);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetTerrify(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsTerrify) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetTerrify(bInIsTerrify);
            }
        }
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetCriticalHit(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsCriticalHit();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetBleed(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsBleed();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetBurn(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsBurn();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetPoison(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsPoison();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetStun(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsStun();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetSlow(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsSlow();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetWeaken(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsWeaken();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetFreeze(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsFreeze();
            }
        }
        return false;
    }

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetTerrify(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsTerrify();
            }
        }
        return false;
    }
};

#undef MYTHIC_CONTEXT_BOOL_SETTER
#undef MYTHIC_CONTEXT_BOOL_PROPERTY
