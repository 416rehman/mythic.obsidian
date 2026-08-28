
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "AttackFragment.generated.h"

class UAnimMontage;
class UMythicAbilitySystemComponent;
class UMythicAnimNotify_SphereOverlap;
class UMythicWeaponAttackAbility;

/** Immutable runtime view of one authored attack variant in a montage. */
struct FAttackRuntimeSectionDescriptor {
    FName SectionName = NAME_None;
    float DurationSeconds = 0.0f;
    TArray<TWeakObjectPtr<UMythicAnimNotify_SphereOverlap>> AuthorizedHitSamples;
};

/** Per-fragment cache compiled from immutable montage content on first use. */
struct FAttackRuntimeContractCache {
    TWeakObjectPtr<UAnimMontage> Montage;
    /** Live global scale used to build this entry; a mutation must invalidate even packaged O(1) caches. */
    float ValidatedMontageRateScale = 0.0f;
    bool bBuilt = false;
    bool bValid = false;
    float NominalCycleDurationSeconds = 0.0f;
    FText Error;
    TArray<FAttackRuntimeSectionDescriptor> Sections;
    TMap<FName, int32> SectionIndexByName;
};

/** Authority-only bookkeeping for the attack ability granted while this item is active. */
USTRUCT()
struct FAttackRuntimeServerOnlyData {
    GENERATED_BODY()

    /** Ability system that owns AbilityHandle; never replicated or persisted. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicAbilitySystemComponent> ASC = nullptr;

    /** Handle of the attack ability granted by this fragment while active. */
    UPROPERTY(Transient)
    FGameplayAbilitySpecHandle AbilityHandle;

};

/** Immutable attack ability and montage configuration authored on a weapon's item definition. */
USTRUCT(Blueprintable, BlueprintType)
struct FAttackConfig {
    GENERATED_BODY()

    /**
     * Canonical native weapon-attack ability granted while this item is active. The typed base seals damage cadence,
     * hit multiplicity, prediction, and montage lifecycle; Blueprint children may only customize presentation and
     * target resolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack",
              meta = (AllowAbstract = "false",
                      ToolTip = "Weapon attack ability whose native base owns commit, cadence, hit count, damage dispatch, and montage lifecycle."))
    TSubclassOf<UMythicWeaponAttackAbility> TriggerAbility = nullptr;

    /**
     * Authored attack-variant montage played by the attack ability; each montage section is one complete attack
     * cycle, selected uniformly at runtime, and GAS AttackSpeed exclusively modifies that selected section's play
     * rate. The montage asset's global Rate Scale must remain exactly 1.0 so combat cadence and DPS presentation
     * cannot diverge through a hidden second multiplier.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack",
              meta = (ToolTip = "Attack montage whose standalone sections are uniformly selected variants with one or more authorized temporal hit samples and one damage budget per section. Its global Rate Scale must be exactly 1.0; GAS AttackSpeed owns all runtime cadence scaling."))
    TObjectPtr<UAnimMontage> AttackMontage = nullptr;
};

/** Instanced weapon fragment that grants the canonical attack ability and validates its montage contract. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UAttackFragment : public UActionableItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Attack)

    /**
     * Definition-authored attack configuration replicated to the owning client. It is intentionally excluded from
     * SaveGame so loading clones the current Item Definition values instead of reviving stale immutable configuration.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly,
              meta = (ShowOnlyInnerProperties,
                      ToolTip = "Live definition-authored attack configuration. Save data rehydrates this from the current Item Definition."))
    FAttackConfig AttackConfig = FAttackConfig();

    /** Transient authority-only ability bookkeeping; never replicated, persisted, or exposed to Blueprint. */
    UPROPERTY(Transient, DuplicateTransient)
    FAttackRuntimeServerOnlyData AttackRuntimeServerOnlyData = FAttackRuntimeServerOnlyData();

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif

    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    /** Returns whether activation must grant an attack ability rather than reuse the currently live handle. */
    static bool ShouldGrantAttackAbility(bool bAbilityHandleValid) { return !bAbilityHandleValid; }

    /** True: the equipped weapon is the only item that grants an attack, so its ability binds the attack input. */
    static bool ShouldBindAbilityToGenericInput(const UMythicItemInstance *ItemInstance);

    /**
     * Returns true only when the montage has the canonical global playback scale of exactly 1.0. Weapon cadence
     * has one owner: the GAS attack-speed play rate. Zero, non-finite, and any non-unit asset multiplier fail closed.
     */
    static bool HasCanonicalMontageRateScale(const UAnimMontage *AttackMontage);

    /**
     * Returns the uniformly weighted mean duration of the montage's attack-variant sections. This is the base attack
     * cycle consumed by both combat activation and item DPS presentation; zero means the montage contract is invalid,
     * including a montage whose global Rate Scale is not exactly 1.0.
     */
    static float GetNominalAttackCycleDuration(const UAnimMontage *AttackMontage);

    /**
     * Validates the complete authored montage contract: standalone named sections with one or more authorized
     * temporal hit samples in every section. A section still owns one damage budget regardless of sample count.
     */
    static bool IsAttackMontageContractValid(const UAnimMontage *AttackMontage,
                                             FText *OutError = nullptr);

    /**
     * Returns whether one authored temporal sample is safe for the canonical server overlap: exact sealed native
     * class and event tag, finite positive radius and offset, and a non-negative target cap.
     */
    static bool IsCanonicalHitSampleUsable(
        const UMythicAnimNotify_SphereOverlap *HitSample,
        FText *OutError = nullptr);

    /**
     * Validates and collects every authorized temporal hit sample that can fire while SectionName plays, including
     * notifies authored in montage slot sequences. One unusable sphere sample invalidates the complete section.
     */
    static bool FindCanonicalHitNotifiesForSection(
        const UAnimMontage *AttackMontage, FName SectionName,
        TArray<const UMythicAnimNotify_SphereOverlap *> &OutNotifies,
        FText *OutError = nullptr);

    /** Builds or reuses the packaged-content attack descriptor; repeated activations are O(1). */
    bool ResolveRuntimeAttackContract(FText *OutError = nullptr) const;

    /** Returns the cached uniformly weighted base attack cycle, or zero when the live contract is invalid. */
    float GetRuntimeNominalAttackCycleDuration() const;

    /** Copies the cached section names used for prediction-stable transient variant selection. */
    bool GetRuntimeAttackSectionNames(TArray<FName> &OutSectionNames,
                                      FText *OutError = nullptr) const;

    /** Copies the cached authorized temporal hit samples for one selected attack section. */
    bool GetRuntimeAuthorizedHitSamples(
        FName SectionName,
        TArray<const UMythicAnimNotify_SphereOverlap *> &OutSamples,
        FText *OutError = nullptr) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, AttackConfig, COND_InitialOrOwner);
    }

private:
    mutable FAttackRuntimeContractCache RuntimeContractCache;
};
