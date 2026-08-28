#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

class UAbilitySystemComponent;

/** Ordered permanent-source layers composed below temporary Gameplay Effects. */
enum class EMythicPermanentStatContributionLayer : uint8 {
    Progression,
    Equipment
};

/** One resolved, immutable contribution to the server-owned permanent stat layer. */
struct MYTHIC_API FMythicPermanentStatContribution {
    /** Stable source identity; an affix Roll Guid or a reward/progression source Guid. */
    FGuid SourceGuid;
    FGameplayAttribute Attribute;
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::AddBase;
    float Magnitude = 0.0f;
    EMythicPermanentStatContributionLayer Layer = EMythicPermanentStatContributionLayer::Equipment;
};

/** Result details for an atomic permanent-stat-layer reconciliation. */
struct MYTHIC_API FMythicPermanentStatReconcileResult {
    /** True only when every target base was written and verified. */
    bool bSucceeded = false;

    /** False means a failed transaction could not restore every pre-transaction GAS base. */
    bool bRollbackSucceeded = true;

    /** Non-localized diagnostic intended for logs and tests. */
    FString Error;
};

/** Immutable presentation snapshot of one attribute's permanent source layers. */
struct MYTHIC_API FMythicPermanentStatLayerSnapshot {
    FGameplayAttribute Attribute;
    float NonEquipmentBase = 0.0f;
    float EquipmentBase = 0.0f;
};

/**
 * Deterministic permanent equipment/stat-source layer over GAS base attributes.
 *
 * The ledger owns no GameplayEffects. It composes all permanent affix modifiers into the GAS base, leaving ordinary
 * duration/infinite GameplayEffects as a separate temporary layer evaluated by GAS above that base. For every
 * attribute it retains the pre-source baseline, composes source-addressed progression rewards, then composes
 * equipment. Any out-of-band change while a stat is tracked rejects the next transaction instead of being silently
 * absorbed into an unspecified layer.
 *
 * ReconcileTransactional validates the complete desired set before writing, verifies every write, and restores all
 * pre-transaction bases on any failure. The caller must quarantine its source ledger if rollback itself fails.
 */
class MYTHIC_API FMythicPermanentStatLedger {
public:
    /** Atomically replaces the complete permanent contribution set on an authoritative ASC. */
    bool ReconcileTransactional(
        UAbilitySystemComponent &AbilitySystem,
        TConstArrayView<FMythicPermanentStatContribution> DesiredContributions,
        FMythicPermanentStatReconcileResult &OutResult);

    /** Atomically restores every tracked attribute to its external base and clears the permanent layer. */
    bool ClearTransactional(
        UAbilitySystemComponent &AbilitySystem,
        FMythicPermanentStatReconcileResult &OutResult);

    /** Drops bookkeeping without touching GAS; used only after the bound ASC is already unavailable/destroyed. */
    void Abandon();

    bool IsEmpty() const { return Attributes.IsEmpty(); }
    int32 GetTrackedAttributeCount() const { return Attributes.Num(); }

    /** Returns the non-equipment and equipment-composed bases in deterministic attribute order. */
    void GetLayerSnapshots(TArray<FMythicPermanentStatLayerSnapshot> &OutSnapshots) const;

    /** Pure deterministic GAS-channel composition used by runtime and focused automation tests. */
    static bool Compose(
        float ExternalBase,
        TConstArrayView<FMythicPermanentStatContribution> Contributions,
        float &OutComposedBase);

private:
    struct FAttributeState {
        float BaselineBase = 0.0f;
        float NonEquipmentBase = 0.0f;
        float LastComposedBase = 0.0f;
        TArray<FMythicPermanentStatContribution> Contributions;
    };

    TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
    TMap<FGameplayAttribute, FAttributeState> Attributes;
};
