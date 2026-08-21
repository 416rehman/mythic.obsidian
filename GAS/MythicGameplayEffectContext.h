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

    UFUNCTION(BlueprintCallable, Category = "Mythic|GAS|GameplayEffectContext")
    static void SetDodged(UPARAM(ref) FGameplayEffectContextHandle &ContextHandle, bool bInIsDodged) {
        if (ContextHandle.IsValid()) {
            auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle.Get());
            if (MythicContext) {
                MythicContext->SetDodged(bInIsDodged);
            }
        }
    }

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
