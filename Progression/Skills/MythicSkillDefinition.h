
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicAbilityActionTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicSkillDefinition.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EMythicAbilityAcquisition : uint8 { Starter, RewardOnly, Prerequisite };

/** Reuse is explicit: a resource or environment gate need not invent a timed cooldown. */
UENUM(BlueprintType)
enum class EMythicAbilityReusePolicy : uint8 { TimedCooldown, NoTimedCooldown };

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAbilityNumber {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag Key;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Value = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Minimum = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Maximum = 1;
};

/**
 * One authored choice a skill offers as it levels. A point buys it, and while it is active its deltas are folded
 * into the same quantifiers the ability already resolves, on top of the caster's bonus attributes rather than
 * instead of them.
 *
 * Every delta is signed, so a modifier can trade one quantifier away to buy another: a wider arc that reaches
 * fewer targets, a longer dash that shortens the stance behind it.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSkillModifier {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FName ModifierId;
    /** Optional durable achievement/unlock/story fact. Empty means included with learned ownership. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FGameplayTag RequiredUnlockTag;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FName ExclusiveBehaviorGroup;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FGameplayTagContainer BehaviorFlags;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FGameplayTagContainer AddClassificationTags;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") FGameplayTagContainer RemoveClassificationTags;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") TMap<FGameplayTag, float> NumberDeltas;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") TMap<FName, float> ParameterDeltas;
    /** Replacement is local to this action; never adds a ranged facet to every hit in the ability. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier") TArray<FMythicAbilityActionProfile> ActionOverrides;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
    FText Name;

    // Drawn by a RichTextBlock, so style markup in the authored string is live.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier")
    TSoftObjectPtr<UTexture2D> Icon;

    // Points to unlock. Never free: a modifier that cost nothing would be taken by every build without a decision.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier", meta = (ClampMin = "1"))
    int32 PointCost = 1;

    // Centimetres added to the shape's Radius. A negative delta shrinks the skill, it never inverts it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas")
    float RadiusDelta = 0.0f;

    // Targets added to the shape's MaxTargets. An uncapped skill (MaxTargets 0) stays uncapped and ignores this.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas")
    int32 TargetCountDelta = 0;

    // Seconds added to SelfEffectDuration. Ignored by a skill that keeps the effect's own authored duration.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas")
    float DurationDelta = 0.0f;

    // Centimetres added to MovementDistance. Ignored by a skill that does not move you.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas")
    float MovementDistanceDelta = 0.0f;

    // Status.Type.* replacing what the skill inflicts - NOT the GAS.Debuff.* state the status grants once it lands.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas", meta = (Categories = "Status.Type"))
    FGameplayTag StatusOverride;

    // Fraction added to StatusChance, matching its 0-1 units. 0.25 is a quarter more often, not a quarter as often.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifier|Deltas", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float StatusChanceDelta = 0.0f;

    // False means the modifier costs a point and changes nothing. Content validation and tests fail on it.
    bool HasEffect() const {
        return !FMath::IsNearlyZero(RadiusDelta)
            || TargetCountDelta != 0
            || !FMath::IsNearlyZero(DurationDelta)
            || !FMath::IsNearlyZero(MovementDistanceDelta)
            || !FMath::IsNearlyZero(StatusChanceDelta)
            || StatusOverride.IsValid() || !BehaviorFlags.IsEmpty() || !NumberDeltas.IsEmpty()
            || !AddClassificationTags.IsEmpty() || !RemoveClassificationTags.IsEmpty()
            || !ParameterDeltas.IsEmpty() || !ActionOverrides.IsEmpty();
    }
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicSkillDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    EMythicAbilityAcquisition Acquisition = EMythicAbilityAcquisition::RewardOnly;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (AssetBundles = "Runtime"))
    TSoftClassPtr<UGameplayAbility> RuntimeAbility;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Reuse")
    EMythicAbilityReusePolicy ReusePolicy = EMythicAbilityReusePolicy::TimedCooldown;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTag CooldownTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TSubclassOf<UGameplayEffect> CooldownEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (ClampMin = "0", ClampMax = "3600"))
    float CooldownSeconds = 1;

    UFUNCTION(BlueprintPure, Category = "Skill|Reuse")
    bool HasTimedCooldown() const { return ReusePolicy == EMythicAbilityReusePolicy::TimedCooldown; }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TArray<FMythicAbilityNumber> Numbers;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Actions", meta=(TitleProperty="ActionId"))
    TArray<FMythicAbilityActionProfile> Actions;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Actions", meta=(TitleProperty="Name"))
    TArray<FMythicAbilityParameter> Parameters;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTagContainer BehaviorFlags;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FText Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TSoftObjectPtr<UTexture2D> Icon;

    // Drawn by a RichTextBlock, so style markup in the authored string is live.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = true))
    FText Description;

    // Active. The slot it sits in decides which key fires it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TSubclassOf<UGameplayAbility> Ability;

    // Permanent fact required only by Acquisition=Prerequisite. RewardOnly ownership comes from reward delivery.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTag RequiredTag;

    // Shown while the skill is locked: how RequiredTag is earned.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = true))
    FText Hint;

    // Skill.Kind.* — AoE, Defensive, Ranged, Projectile, Summon, Movement. Runes, talents and affixes key off these.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTagContainer ClassificationTags;

    /**
     * What practice lets the player choose. ModifierId is the persistent identity; rows may be reordered.
     * Keep existing IDs stable when editing content so saved choices retain their meaning.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (TitleProperty = "Name"))
    TArray<FMythicSkillModifier> Modifiers;

    /** Compatibility only; new abilities use independent mastery and explicit refinement eligibility. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Legacy") bool bUseLegacyModifierProgression = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Mastery") bool bHasMasteryTrack = false;
    /** Strictly increasing cumulative accepted uses. Rank ceiling is one plus this array's size. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Mastery") TArray<int32> MasteryUseThresholds;
    /** Zero uses only authored conflict groups. No global four-choice cap for unrelated behaviors. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Refinements", meta=(ClampMin="0",ClampMax="50")) int32 RefinementSelectionLimit = 0;

    bool HasPayload() const { return !RuntimeAbility.IsNull() || Ability != nullptr; }
    TSubclassOf<UGameplayAbility> GetLoadedAbility() const { return RuntimeAbility.IsNull() ? Ability : TSubclassOf<UGameplayAbility>(RuntimeAbility.Get()); }
    int32 FindModifier(FName Id) const;
    bool ValidateDefinition(TArray<FText>& Errors) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
