
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Components/ActorComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "ProficiencyComponent.generated.h"

struct FMilestone;
class UProficiencyDefinition;

/** Replication-safe, presentation-ready snapshot of one player's proficiency progress. */
USTRUCT(BlueprintType)
struct FProficiencySummary {
    GENERATED_BODY()

    /** Localized player-facing name of the proficiency track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText Name;

    /** Stable track identity used for grouping and lookup without matching localized display text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FGameplayTag TrackTag;

    /** Localized player-facing description of the proficiency track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText Description;

    /** Current level reached on this track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    int32 Level = 0;

    /** Current lifetime XP accumulated on this track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float CurrentXP = 0.0f;

    /** Lifetime XP threshold at which the current level began. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float LevelXPStart = 0.0f;

    /** Lifetime XP threshold required to reach the next level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float LevelXPEnd = 0.0f;

    /** Normalized progress from the current level to the next in the range 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float ProgressFraction = 0.0f;

    /** Next named milestone ahead of the player; empty after every milestone has been earned. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText NextMilestoneName;

    /** Level of the next named milestone, or zero when the track has no remaining milestone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    int32 NextMilestoneLevel = 0;

    /** Soft player-facing icon copied from the definition for presentation without loading the definition in UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    TSoftObjectPtr<UTexture2D> Icon;
};

/** Runtime state for one typed proficiency definition and its transiently compiled reward track. */
USTRUCT(BlueprintType, Blueprintable)
struct FProficiency {
    GENERATED_BODY()

    void Instantiate();
    void GenerateTrack();

    /** Resolves the current-XP GAS attribute from the definition's canonical Progress Stat. */
    FGameplayAttribute GetProgressAttribute() const;

    /** Resolves the Max-XP GAS attribute from the Progress Stat's typed capacity pair. */
    FGameplayAttribute GetProgressCapacityAttribute() const;

    /** Sole authored identity and tuning source for this proficiency track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TObjectPtr<UProficiencyDefinition> Definition = nullptr;

    /** Transient compiled milestone/reward track rebuilt from Definition; never saved or replicated as authority. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, NotReplicated, Category = "Proficiency")
    TArray<FMilestone> Track = TArray<FMilestone>();

    float SavedXP = 0.0f;

    float MaxXP = 0.0f;
};

/** Authority-owned, owner-replicated proficiency progression and reward orchestration component. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UProficiencyComponent : public UActorComponent {
    GENERATED_BODY()

public:
    /** Authored proficiency roster; each entry must reference one unique definition and Progress Stat. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TArray<FProficiency> Proficiencies;

    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> ASC;

    /**
     * Authority-only restore of the complete authored roster after canonical Stat Definitions are ready. The call
     * validates the roster before mutation, writes staged XP, atomically replaces source-addressed permanent stat
     * rewards, and replays only load-safe non-attribute rewards; repeated calls are idempotent.
     */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void ApplyLoadedProficiencies();

    /** Authority-only convenience entry point that grants scaled XP to the configured Combat proficiency. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantCombatXP(float Amount);

    /**
     * Authority-only generic XP grant for an authored proficiency definition. Invalid amounts, unknown definitions,
     * and non-authority calls are ignored; valid grants include the player's proficiency XP multipliers.
     */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantProficiencyXP(UProficiencyDefinition *Definition, float Amount);

    /**
     * As GrantProficiencyXP, but the caller also describes WHAT the work was.
     *
     * ContextTags ride the emitted GAS.Event.Proficiency.Gained payload alongside the track tag, so a proc rule can
     * gate on them via RequiredEventTag. The track tag turns one event into twelve; this turns each of those into
     * as many as the caller can distinguish -- the station that did the work, the quality tier that came out, the
     * kind of thing gathered.
     *
     * Callers already hold this information at the moment they grant XP and currently discard it. Nothing is
     * inferred here: a caller that passes nothing produces exactly the payload it produced before.
     */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantProficiencyXPWithContext(UProficiencyDefinition *Definition, float Amount, FGameplayTagContainer ContextTags);

    /**
     * Native authority seam used by durable delivery. Returns true only when the exact typed track consumed the
     * finite positive amount; reaching the track cap still counts as consumption so a receipt cannot retry forever.
     */
    bool TryGrantProficiencyXPWithContext(
        UProficiencyDefinition *Definition,
        float Amount,
        const FGameplayTagContainer &ContextTags);

    /** Reduces Combat proficiency XP by a normalized penalty fraction on authority without crossing the level floor. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void ApplyDeathPenalty(float PenaltyFraction);

    static float ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction);

    static float ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction, float LevelFloorXP);

    FProficiency* FindCombatProficiency();

    static float ComputeXpOverflow(float CurrentXP, float Amount, float MaxXP);

    bool IsRestoring() const { return bIsRestoring; }

    /** Returns display-ready progress and milestone data for one roster index, or an empty summary if invalid. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    FProficiencySummary GetSummary(int32 Index) const;

    /**
     * Resolves the live level for one exact Proficiency Definition without tag/name lookup. Returns false for a
     * missing/duplicated roster entry or invalid XP state and never mutates progression.
     */
    bool TryGetLevelForDefinition(
        const UProficiencyDefinition *Definition, int32 &OutLevel) const;

protected:
    bool bIsRestoring = false;
    bool bSemanticDataRequestPending = false;

    void OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData);
    bool ConfigureProgressionAttribute(FProficiency &Proficiency);

    void ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel);

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
