#pragma once

#include "CoreMinimal.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"

class UMythicItemizationDataRegistrySubsystem;

/** Per-row eligibility diagnostics for one deterministic generation decision. */
struct MYTHIC_API FMythicAffixCandidateTrace {
    FGuid SliceGuid;
    int32 SlicePriority = 0;
    int32 SliceWeight = 0;
    FPrimaryAssetId PoolId;
    FGuid PoolRowGuid;
    FMythicAffixDefinitionHandle AffixDefinition;
    FGameplayTag RollGroup;
    int64 CompiledRowWeight = 0;
    int64 MinimumEligibleTierCost = 0;
    bool bEligible = false;
    FName RejectionCode;
};

/** Numeric draw diagnostics for the owning Affix Definition's single value. */
struct MYTHIC_API FMythicAffixValueTrace {
    uint64 Draw = 0;
    float ResolvedMin = 0.0f;
    float ResolvedMax = 0.0f;
    float ValueBeforeQuantization = 0.0f;
    float FinalValue = 0.0f;
};

/** Full deterministic audit of one landed or failed roll. */
struct MYTHIC_API FMythicAffixRollDecisionTrace {
    int32 RollOrdinal = 0;
    TArray<FMythicAffixCandidateTrace> Candidates;
    uint64 SliceSelectionDraw = 0;
    uint64 RowSelectionDraw = 0;
    uint64 TierSelectionDraw = 0;
    FGuid SelectedSliceGuid;
    FGuid SelectedPoolRowGuid;
    FMythicAffixDefinitionHandle SelectedAffixDefinition;
    FName SelectedProgression;
    int32 SelectedTierRank = 0;
    FMythicAffixValueTrace Value;
    int64 BudgetBefore = 0;
    int64 BudgetAfter = 0;
    FGuid ResultRollGuid;
};

/** Complete server-side generation trace for replay, telemetry, and designer diagnostics. */
struct MYTHIC_API FMythicAffixRollTrace {
    FGuid TraceId;
    FPrimaryAssetId ProfileId;
    FPrimaryAssetId PolicyId;
    FMythicContentHash GameplayContentHash;
    FMythicContentHash PresentationContentHash;
    uint64 ServerSeed = 0;
    int32 AlgorithmVersion = 0;
    FGameplayTagContainer RequestContext;
    TArray<FMythicAffixRollDecisionTrace> Decisions;
    TArray<FName> Diagnostics;
};

/** Runtime context for one guaranteed affix materialization. */
struct MYTHIC_API FMythicAffixGrantContext {
    FGuid ItemInstanceGuid;
    int32 ItemLevel = 1;
    EItemRarity Rarity = EItemRarity::Common;
    FGameplayTagContainer ContextTags;
    uint64 Seed = 0;
    FPrimaryAssetId ProfileId;
    FPrimaryAssetId PolicyId;
    FPrimaryAssetId PoolId;
    int32 RollOrdinal = 0;
};

/** Complete deterministic request for one concrete Affix Profile. */
struct MYTHIC_API FMythicAffixRollRequest {
    FGuid ItemInstanceGuid;
    int32 ItemLevel = 1;
    EItemRarity Rarity = EItemRarity::Common;
    FGameplayTagContainer ContextTags;
    FPrimaryAssetId ProfileId;
    uint64 Seed = 0;
    int32 AlgorithmVersion = 1;
};

/** Materializes already-compiled direct grants used by gems, sockets, and crafting. */
struct MYTHIC_API FMythicAffixGrantService {
    static bool Materialize(const FMythicAffixGrantSpec &Spec,
                            const FMythicAffixGrantContext &Context,
                            const UMythicItemizationDataRegistrySubsystem &Registry,
                            FRolledAffix &OutSnapshot,
                            FMythicAffixRollTrace *OptionalTrace);
};

/** Deterministic, server-authoritative profile generator. */
struct MYTHIC_API FMythicAffixGenerator {
    static bool Generate(const FMythicAffixRollRequest &Request,
                         const FCompiledAffixProfile &Profile,
                         TArray<FRolledAffix> &OutSnapshots,
                         FMythicAffixRollTrace *OptionalTrace);
};
