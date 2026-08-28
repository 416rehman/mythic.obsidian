// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"
#include "MythicAbilitySourceInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MythicGameplayEffectContext.generated.h"

class AActor;
class AController;
class APawn;
class APlayerState;
class FArchive;
class UObject;
class UPhysicalMaterial;

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

    static FMythicGameplayEffectContext *ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

    /**
     * Pins the authoritative ASC used as this context's GAS source identity. This is the invariant fallback for a
     * truly actor-less world status whose stack is owned by the GameState ASC; actor hazards own transient source ASCs.
     */
    void SetInstigatorAbilitySystemComponentForStacking(UAbilitySystemComponent *SourceASC) {
        InstigatorAbilitySystemComponent = SourceASC;
    }

    void SetAbilitySource(const IMythicAbilitySourceInterface *InObject, float InSourceLevel);

    const IMythicAbilitySourceInterface *GetAbilitySource() const;
    const UPhysicalMaterial *GetPhysicalMaterial() const;

    MYTHIC_CONTEXT_BOOL_PROPERTY(CriticalHit)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Bleed)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Burn)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Poison)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Stun)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Slow)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Weaken)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Freeze)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Terrify)

    MYTHIC_CONTEXT_BOOL_PROPERTY(Dodged)

protected:
    UPROPERTY()
    FString ApplierPlayerKey;

public:
    const FString &GetApplierPlayerKey() const { return ApplierPlayerKey; }
    void SetApplierPlayerKey(const FString &InApplierPlayerKey) { ApplierPlayerKey = InApplierPlayerKey; }

protected:
    UPROPERTY()
    float ShieldAbsorbed = 0.0f;

public:
    // How much of this hit the target's shield ate. The on-hit chain adds it back so a fully absorbed hit
    // still counts as a hit.
    float GetShieldAbsorbed() const { return ShieldAbsorbed; }
    void SetShieldAbsorbed(float InShieldAbsorbed) { ShieldAbsorbed = InShieldAbsorbed; }

    virtual FGameplayEffectContext *Duplicate() const override {
        FMythicGameplayEffectContext *NewContext = new FMythicGameplayEffectContext();
        *NewContext = *this;
        if (GetHitResult()) {
            NewContext->AddHitResult(*GetHitResult(), true);
        }
        return NewContext;
    }

    virtual UScriptStruct *GetScriptStruct() const override {
        return StaticStruct();
    }

    virtual bool NetSerialize(FArchive &Ar, class UPackageMap *Map, bool &bOutSuccess) override;

protected:
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
    /**
     * Resolves the pawn, controller and player state behind whatever GAS names as an instigator. A player
     * instigates from their PlayerState because that owns their ASC, an NPC from its pawn, so a cast chain
     * that only handles pawns and controllers silently drops every player.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|GAS|GameplayEffectContext")
    static void ResolveInstigator(AActor *Instigator, APawn *&OutPawn, AController *&OutController, APlayerState *&OutPlayerState);

    /** Marks the effect context as a critical hit. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetCriticalHit(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsCriticalHit) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetCriticalHit(bInIsCriticalHit);
            }
        }
    }

    /** Marks the effect context as applying bleed. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetBleed(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsBleed) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetBleed(bInIsBleed);
            }
        }
    }

    /** Marks the effect context as applying burn. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetBurn(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsBurn) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetBurn(bInIsBurn);
            }
        }
    }

    /** Marks the effect context as applying poison. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetPoison(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsPoison) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetPoison(bInIsPoison);
            }
        }
    }

    /** Marks the effect context as applying stun. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetStun(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsStun) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetStun(bInIsStun);
            }
        }
    }

    /** Marks the effect context as applying slow. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetSlow(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsSlow) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetSlow(bInIsSlow);
            }
        }
    }

    /** Marks the effect context as applying weaken. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetWeaken(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsWeaken) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetWeaken(bInIsWeaken);
            }
        }
    }

    /** Marks the effect context as applying freeze. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetFreeze(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsFreeze) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetFreeze(bInIsFreeze);
            }
        }
    }

    /** Marks the effect context as applying terrify. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetTerrify(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsTerrify) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetTerrify(bInIsTerrify);
            }
        }
    }

    /** Returns whether the effect context represents a critical hit. */
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

    /** Returns whether the effect context applies bleed. */
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

    /** Returns whether the effect context applies burn. */
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

    /** Returns whether the effect context applies poison. */
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

    /** Returns whether the effect context applies stun. */
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

    /** Returns whether the effect context applies slow. */
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

    /** Returns whether the effect context applies weaken. */
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

    /** Returns whether the effect context applies freeze. */
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

    /** Returns whether the effect context applies terrify. */
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

    /** Marks the effect context as a dodged hit. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetDodged(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsDodged) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetDodged(bInIsDodged);
            }
        }
    }

    /** Returns whether the effect context represents a dodged hit. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static bool GetDodged(const FGameplayEffectContextHandle &ContextHandle) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<const FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                return MythicContext->IsDodged();
            }
        }
        return false;
    }
};

#undef MYTHIC_CONTEXT_BOOL_SETTER
#undef MYTHIC_CONTEXT_BOOL_PROPERTY
