
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Components/ActorComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "ProficiencyComponent.generated.h"

struct FMilestone;
class UProficiencyDefinition;

USTRUCT(BlueprintType)
struct FProficiencySummary {
    GENERATED_BODY()

    // name of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText Name;

    // description of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText Description;

    // current level reached
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    int32 Level = 0;

    // current cumulative experience
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float CurrentXP = 0.0f;

    // cumulative experience required for the current level
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float LevelXPStart = 0.0f;

    // cumulative experience required for the next level
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float LevelXPEnd = 0.0f;

    // progress fraction towards the next level
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    float ProgressFraction = 0.0f;

    // The next key milestone ahead of the current level. Empty once every milestone on the track is earned.
    // Levelling a track is only worth doing if you can see what it is building towards, so this rides along
    // with the summary rather than making every caller walk the generated track itself.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    FText NextMilestoneName;

    // The level that milestone lands on. 0 when there is none left.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    int32 NextMilestoneLevel = 0;

    // The track's mark, copied off the definition. The UI reads summaries and never the definition, so without
    // this the icon simply cannot reach the page.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Summary")
    TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType, Blueprintable)
struct FProficiency {
    GENERATED_BODY()

    void Instantiate();
    void GenerateTrack();

    // The definition of the proficiency
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    UProficiencyDefinition *Definition = nullptr;

    // The ASC attribute indicating the progress of this proficiency
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    FGameplayAttribute ProgressAttribute = FGameplayAttribute();

    // The generated proficiency track
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TArray<FMilestone> Track = TArray<FMilestone>();

    float SavedXP = 0.0f;

    float MaxXP = 0.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UProficiencyComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TArray<FProficiency> Proficiencies;

    UPROPERTY()
    UAbilitySystemComponent *ASC;

    /**
     * SERVER: apply staged FProficiency::SavedXP to the ASC for every proficiency (Instantiate ->
     * ConfigureProgressionAttribute -> clamp -> SetNumericAttributeBase -> ReapplyRewardsForLevel), guarded by
     * bIsRestoring. Authority only. Called from BeginPlay (cold start) AND from the save-load finished path
     * (CharacterData::Deserialize) — the latter is required because async character load completes AFTER BeginPlay,
     * so without a re-apply the loaded XP would stage into the struct but never reach the ASC. Idempotent:
     * SetNumericAttributeBase is an absolute set and only CanReapplyOnLoad rewards are re-Given.
     */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void ApplyLoadedProficiencies();

    // server: grant combat proficiency XP to the owning player (applies ProficiencyXPBonus and Enlighten scaling)
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantCombatXP(float Amount);

    // server: grant XP to ANY proficiency by its definition (the generic primitive — used by crafting completion,
    // reusable for gathering/etc). No-op off-authority, Amount<=0, null Def, or a Def this player has no proficiency for.
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantProficiencyXP(UProficiencyDefinition *Definition, float Amount);

    /**
     * As GrantProficiencyXP, but the caller also describes WHAT the work was.
     *
     * ContextTags ride the emitted GAS.Event.Proficiency.Gained payload alongside the track tag, so a proc rule can
     * gate on them via RequiredEventTags. The track tag turns one event into twelve; this turns each of those into
     * as many as the caller can distinguish -- the station that did the work, the quality tier that came out, the
     * kind of thing gathered.
     *
     * Callers already hold this information at the moment they grant XP and currently discard it. Nothing is
     * inferred here: a caller that passes nothing produces exactly the payload it produced before.
     */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void GrantProficiencyXPWithContext(UProficiencyDefinition *Definition, float Amount, FGameplayTagContainer ContextTags);

    // server: apply death penalty to combat proficiency XP (reduces current XP by PenaltyFraction)
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void ApplyDeathPenalty(float PenaltyFraction);

    static float ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction);

    static float ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction, float LevelFloorXP);

    FProficiency* FindCombatProficiency();

    static float ComputeXpOverflow(float CurrentXP, float Amount, float MaxXP);

    bool IsRestoring() const { return bIsRestoring; }

    // returns a summary of the proficiency at the specified index
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    FProficiencySummary GetSummary(int32 Index) const;

protected:
    bool bIsRestoring = false;

    void OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData);
    void ConfigureProgressionAttribute(FProficiency &Proficiency);

    void ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel);

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
