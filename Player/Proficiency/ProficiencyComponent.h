// 

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

    // XP value loaded from save (staged here until BeginPlay when ASC is ready)
    float SavedXP = 0.0f;

    // Cache of the max XP for this proficiency (calculated at runtime)
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

    // server: apply death penalty to combat proficiency XP (reduces current XP by PenaltyFraction)
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    void ApplyDeathPenalty(float PenaltyFraction);

    // pure death penalty math: returns XP left after losing PenaltyFraction of current progress
    static float ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction);

    // find the proficiency whose ProgressAttribute matches CombatProficiency
    FProficiency* FindCombatProficiency();

    // check if the component is restoring loaded values
    bool IsRestoring() const { return bIsRestoring; }

    // returns a summary of the proficiency at the specified index
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    FProficiencySummary GetSummary(int32 Index) const;

protected:
    // When true, skip normal reward logic in OnAttributeChanged (used during restore)
    bool bIsRestoring = false;

    void OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData);
    void ConfigureProgressionAttribute(FProficiency &Proficiency);

    /** Reapply only CanReapplyOnLoad rewards for levels up to given level */
    void ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel);

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
