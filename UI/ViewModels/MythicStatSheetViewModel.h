// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "AttributeSet.h"
#include "MythicStatDisplay.h"
#include "Stats/MythicStatRegistry.h"
#include "MythicStatSheetViewModel.generated.h"

class UAbilitySystemComponent;
class UMythicItemizationDataRegistrySubsystem;
class UMythicAffixApplicationComponent;
class UMythicAttributeSet;
class UMythicStatSummaryLibrary;

/** Ordered Blueprint-facing stat-sheet section built from one canonical category definition. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatSection {
    GENERATED_BODY()

    /** Localized category heading displayed above this group of stat rows. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Heading;

    /** Canonical category identity used for styling and collapse-state persistence. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag CategoryTag;

    /** Data-authored visual and interaction treatment for this section. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FMythicStatCategoryStyle Style;

    /** Ordered, display-ready stat rows that belong to this category. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    TArray<FMythicStatLine> Lines;
};

/** MVVM projection that builds the complete data-driven stat sheet from live GAS and semantic asset data. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSheetViewModel : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    /** Binds to the local player's Ability System Component and builds the first snapshot; safe to call on every open. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void InitializeForASC(UAbilitySystemComponent *InASC);

    /** Deterministic C++/automation entry point over an already-built semantic registry. */
    void InitializeForASCWithRegistry(UAbilitySystemComponent* InASC, const FMythicStatRegistry& InRegistry);

    /** Releases all Ability System bindings; call when the sheet closes so a hidden panel has no update cost. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void Shutdown();

    /** Forces an immediate snapshot rebuild for changes that did not emit a Gameplay Ability System attribute event. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void Refresh();

    /**
     * What a primary stat is contributing right now, generated from the same authored rows the gameplay
     * reads. The tooltip therefore cannot drift from the maths: retuning a coefficient moves both together.
     *
     * A hand-written tooltip string is the defect this exists to prevent.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats")
    TArray<FMythicStatContributionLine> GetContributionsFor(FGameplayAttribute Stat) const;

    /** True when the category asset enables contribution drill-down for this row. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats")
    static bool SupportsContributionDrilldown(const FMythicStatLine &Line) {
        return Line.bEnableContributionDrilldown;
    }

    /** Complete data-authored stat sheet in category order; categories without visible rows are omitted. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
    TArray<FMythicStatSection> Sections;

    const TArray<FMythicStatSection> &GetSections() const {
        return Sections;
    }

    /** Headline cards computed from the configured summary library and live Ability System values. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
    TArray<FMythicStatSummaryLine> Summaries;

    const TArray<FMythicStatSummaryLine> &GetSummaries() const {
        return Summaries;
    }

    /** Number of stats currently changed by gear, talents, or buffs; drives the sheet's modified-stat summary. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
    int32 ModifiedStatCount = 0;

    int32 GetModifiedStatCount() const {
        return ModifiedStatCount;
    }

    virtual void BeginDestroy() override;

private:
    UFUNCTION()
    void HandleAttributeChanged(const FGameplayAttribute &Attribute, float OldValue, float NewValue);

    void ScheduleRebuild();
    void Rebuild();
    void BindPermanentStatLayer(UAbilitySystemComponent *InASC);
    void HandlePermanentStatLayerChanged();
    void HandleSemanticDataChanged(uint64 SemanticRevision);

    void SetSections(TArray<FMythicStatSection> &&In);
    void SetSummaries(TArray<FMythicStatSummaryLine> &&In);
    void SetModifiedStatCount(int32 In);

    const UMythicStatSummaryLibrary *ResolveSummaryLibrary();

    TWeakObjectPtr<UAbilitySystemComponent> ASC;
    TWeakObjectPtr<UMythicAffixApplicationComponent> AffixApplication;
    FDelegateHandle PermanentStatLayerChangedHandle;

    UPROPERTY()
    TObjectPtr<const UMythicStatSummaryLibrary> SummaryLibrary;

    bool bSummaryLibraryTried = false;

    // Owned and pinned by the GameInstance itemization subsystem (or by the caller in focused tests).
    const FMythicStatRegistry* StatRegistry = nullptr;
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> RegistrySubsystem;
    FDelegateHandle RegistryReadinessHandle;
    FDelegateHandle RegistrySemanticDataChangedHandle;

    UPROPERTY()
    TArray<TWeakObjectPtr<UMythicAttributeSet>> BoundSets;

    bool bRebuildScheduled = false;
};
