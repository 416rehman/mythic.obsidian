
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicSkillDefinition.generated.h"

class UTexture2D;

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
            || StatusOverride.IsValid();
    }
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicSkillDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
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

    // The deed that earns this skill. INVALID = no deed; the skill is available from the first hour.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTag RequiredTag;

    // Shown while the skill is locked: how RequiredTag is earned.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = true))
    FText Hint;

    // Skill.Kind.* — AoE, Defensive, Ranged, Projectile, Summon, Movement. Runes, talents and affixes key off these.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTagContainer ClassificationTags;

    /**
     * What levelling this skill lets you buy. Position is the save key - a character stores the indices it has
     * active - so append to the end and never reorder, or saved characters wake up holding a different choice.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (TitleProperty = "Name"))
    TArray<FMythicSkillModifier> Modifiers;

    bool HasPayload() const { return Ability != nullptr; }
};
