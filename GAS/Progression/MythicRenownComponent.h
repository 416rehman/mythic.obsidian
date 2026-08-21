
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "GAS/Progression/MythicRenownRules.h"
#include "GAS/Progression/MythicRenownTypes.h"
#include "MythicRenownComponent.generated.h"

class UMythicFactionStandingComponent;
class UMythicNarrativeStateComponent;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicOnRenownTierChanged, FGameplayTag, Scope, EMythicRenownTier, NewTier);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicRenownComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicRenownComponent();

    // SERVER: add Amount (signed) to Scope's renown AND the global aggregate. Authority-gated plain call (not a client
    // RPC) — callers derive it from server-owned state (rewards, quest outcomes). On an ascending tier crossing:
    // mirrors the tier story tag(s), gives the tier payload rewards (never while restoring), broadcasts.
    UFUNCTION(BlueprintCallable, Category = "Progression|Renown")
    void ServerGrantRenown(FGameplayTag Scope, float Amount);

    // Pure reads.
    UFUNCTION(BlueprintPure, Category = "Progression|Renown")
    float GetRenown(FGameplayTag Scope) const;

    // The scope's EFFECTIVE tier (tier table + the faction-standing clamp for Faction.* scopes).
    UFUNCTION(BlueprintPure, Category = "Progression|Renown")
    EMythicRenownTier GetTier(FGameplayTag Scope) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Renown")
    float GetVendorDiscount(FGameplayTag Scope) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Renown")
    float GetGlobalRenown() const { return GlobalRenown; }

    UFUNCTION(BlueprintPure, Category = "Progression|Renown")
    EMythicRenownTier GetGlobalTier() const;

    const TArray<FMythicRenownEntry> &GetRenownEntries() const { return RenownEntries; }

    void RestoreRenown(const TArray<FMythicRenownEntry> &SavedEntries, float SavedGlobal);

    // Broadcast (server-side) on every effective tier change. See delegate doc above.
    UPROPERTY(BlueprintAssignable, Category = "Progression|Renown")
    FMythicOnRenownTierChanged OnRenownTierChanged;

    // Component-level override for the tier table; if null, resolved from DeveloperSettings (code defaults when both unset).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Renown")
    TSoftObjectPtr<UMythicRenownTierTable> TierTable;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    // Per-scope renown values. Owner-only: private per-player progression (the UI reads it). In-place mutation
    // replicates on the next net update (mirrors FactionStanding::Standings).
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Progression|Renown")
    TArray<FMythicRenownEntry> RenownEntries;

    // Global aggregate — every scoped grant also feeds this (the account-wide "how renowned are you at all").
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Progression|Renown")
    float GlobalRenown = 0.0f;

private:
    const UMythicRenownTierTable *ResolveTierTable() const;

    TConstArrayView<float> GetThresholds() const;
    TConstArrayView<float> GetVendorDiscounts() const;

    EMythicRenownTier EffectiveTier(const FGameplayTag &Scope, float Value) const;

    void HandleTierChange(const FGameplayTag &Scope, EMythicRenownTier OldTier, EMythicRenownTier NewTier);

    static FGameplayTag MakeTierMirrorTag(const FGameplayTag &Scope, EMythicRenownTier Tier);

    UMythicNarrativeStateComponent *ResolveNarrative() const;
    UMythicFactionStandingComponent *ResolveFactionStanding() const;
    APlayerController *ResolvePC() const;

    bool bIsRestoring = false;

    UPROPERTY(Transient)
    TObjectPtr<UMythicRenownTierTable> ResolvedTierTable = nullptr;
    bool bTableResolved = false;
};
