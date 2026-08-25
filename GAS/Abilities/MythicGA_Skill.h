
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicGA_Skill.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;
class UAnimMontage;

UENUM(BlueprintType)
enum class EMythicSkillShape : uint8 {
    // Everything within Radius of the query origin, whatever direction it lies in. Nova, Shockwave, Maelstrom.
    Sphere,

    // A horizontal slice AngleDegrees wide about the caster's facing. Height is ignored, so a swing reaches a
    // target up a slope exactly as it reaches one on level ground. Cleave, WideSwing.
    Arc,

    // A true cone about the caster's facing, bounded in every direction rather than only in yaw.
    Cone,

    // The nearest thing inside Radius and nothing else. SkillTargetCountBonus is what lets a thrust pierce.
    Single,
};

UENUM(BlueprintType)
enum class EMythicSkillMovement : uint8 {
    None,

    // Root-motion travel over MovementDuration. The movement component owns the move, so the capsule still
    // collides and the same move replays under client prediction rather than arriving by RPC.
    Dash,

    // Instant and server-authoritative. Swept, so it stops at a wall instead of going through one.
    Teleport,
};

/**
 * The volume a skill takes its targets from. The world query is always a sphere of Radius at the origin - the
 * angle narrows what that sphere found, so a wide arc and a full circle cost the same query.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSkillShape {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape")
    EMythicSkillShape Shape = EMythicSkillShape::Sphere;

    // Centimetres, before SkillRadiusBonus. 0 reaches nobody.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0.0"))
    float Radius = 300.0f;

    // Full width of an Arc or Cone, ignored by the other two. 360 is a full turn; 0 is dead ahead only.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0.0", ClampMax = "360.0"))
    float AngleDegrees = 90.0f;

    // Centimetres ahead of the caster the query sits. This is how a skill drops its sphere out in front.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape")
    float ForwardOffset = 0.0f;

    /**
     * Most targets the shape may take, before SkillTargetCountBonus, nearest first. 0 takes everyone it finds -
     * and a target-count bonus has nothing to add to everyone, so it is ignored on an uncapped skill.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shape", meta = (ClampMin = "0"))
    int32 MaxTargets = 0;
};

/**
 * Which candidates a shape takes. Pure geometry over points, with no world and no actors in it, so every arc,
 * cone and cap case is unit-testable: the ability does the overlap, this decides what the overlap meant.
 */
struct MYTHIC_API FMythicSkillTargeting {
    // Cosine bounding an Arc or Cone. AngleDegrees is the full width, so 90 allows 45 either side of forward.
    static float CosineHalfAngle(float AngleDegrees);

    // Where the query sits: ForwardOffset centimetres ahead of the caster.
    static FVector ResolveOrigin(const FMythicSkillShape &Shape, const FVector &ActorLocation, const FVector &Forward);

    // Whether one point is inside the shape. A shape with no radius contains nothing.
    static bool IsInside(const FMythicSkillShape &Shape, const FVector &Origin, const FVector &Forward, const FVector &Point);

    /**
     * Indices of the points the shape takes, nearest to Origin first and trimmed to the shape's MaxTargets.
     * Nearest-first matters wherever the cap bites: a capped skill must keep the targets it is standing on.
     */
    static void SelectTargets(const FMythicSkillShape &Shape, const FVector &Origin, const FVector &Forward,
                              const TArray<FVector> &Points, TArray<int32> &OutSelected);
};

/**
 * Active skill. One class backs all sixteen: what a skill does is the shape it queries, the damage container it
 * applies, the status it inflicts and the way it moves you, all authored on a Blueprint child that carries no
 * graph.
 *
 * Three quantifiers take a bonus attribute off the caster - Radius, MaxTargets and the self-effect duration - so
 * gear moves what a skill does without the skill being re-authored. Shape angle, forward offset, status chance and
 * movement distance are authored-only for now; each needs its own attribute before gear can move it, and adding one
 * means adding the attribute beside the other three rather than borrowing a neighbour's.
 *
 * Damage is deliberately not scaled here. BonusSkillDamage is applied inside the damage execution to any hit tagged
 * GAS.Ability.Type.Skill, which this ability marks its own damage with, so scaling the container here would pay the
 * stat twice.
 */
UCLASS()
class MYTHIC_API UMythicGA_Skill : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill")
    FMythicSkillShape Shape;

    // Leave both effects unset for a skill that deals no damage at all - a stance, a blink, a pure control skill.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill")
    FMythicDamageContainer Damage;

    // Status.Type.* the registry answers to - NOT the GAS.Debuff.* state the status grants once it lands.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill", meta = (Categories = "Status.Type"))
    FGameplayTag StatusToApply;

    // Rolled per target, then again against that target's resistance to the status, as a weapon proc is.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StatusChance = 1.0f;

    // Applied to the caster on activation: a guard, a stance, a buff. The one payload no shape query can express.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill")
    TSubclassOf<UGameplayEffect> SelfEffect;

    // Seconds SelfEffect lasts, before SkillDurationBonus. 0 keeps whatever duration the effect authors itself.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill", meta = (ClampMin = "0.0"))
    float SelfEffectDuration = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill|Movement")
    EMythicSkillMovement Movement = EMythicSkillMovement::None;

    // Centimetres travelled, forward. 0 means the skill does not move you however Movement is set.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill|Movement", meta = (ClampMin = "0.0"))
    float MovementDistance = 0.0f;

    // Seconds a Dash takes to cover MovementDistance. Ignored by Teleport, which is instant.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill|Movement", meta = (ClampMin = "0.01"))
    float MovementDuration = 0.2f;

    // Played at the caster's attack speed. The skill lasts as long as it does.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill")
    TObjectPtr<UAnimMontage> Montage;

    /**
     * Event the payload waits for, so the hit lands on the frame the animation says it does rather than on the
     * frame the key was pressed. A montage raises it - UMythicAnimNotify_SphereOverlap already sends one.
     * Unset means the payload runs the moment the skill starts.
     *
     * Every occurrence runs the payload again, so a montage carrying three notifies is a three-hit skill and
     * each hit queries the shape where the caster is standing at that moment.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Skill")
    FGameplayTag ImpactEventTag;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    // The authored shape with the caster's bonus attributes folded in. What the query actually uses.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    FMythicSkillShape ResolveShape() const;

    // SelfEffectDuration with SkillDurationBonus folded in.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    float ResolveSelfEffectDuration() const;

    // Never negative: a cursed radius shrinks a skill, it does not turn it inside out.
    static float ScaleRadius(float Authored, float Bonus);

    // An uncapped skill (0) stays uncapped. A capped one never falls below a single target.
    static int32 ScaleTargetCount(int32 Authored, float Bonus);

    static float ScaleDuration(float Authored, float Bonus);

    /** Shortest a scaled duration may become. Zero would read as "instant" to GAS. */
    static constexpr float MinScaledDuration = 0.1f;

    // Where a Dash or a Teleport aims. They differ in how they travel, not in where they land.
    static FVector ComputeMovementDestination(const FVector &Start, const FVector &Forward, float Distance);

    bool HasDamage() const { return Damage.DamageCalculationEffect != nullptr || Damage.DamageApplicationEffect != nullptr; }

protected:
    // Authority only. Self effect, then damage and status on everything the shape took.
    void ExecutePayload();

    /**
     * Everything the shape took, nearest first. The world supplies candidates, FMythicSkillTargeting decides
     * which of them the shape actually holds.
     */
    void GatherTargets(const FMythicSkillShape &Scaled, const FVector &Origin, const FVector &Forward, TArray<FHitResult> &OutHits) const;

    void ApplySelfEffect();

    void ApplyStatus(const TArray<FHitResult> &Hits, AActor *Instigator) const;

    // Marks both specs as skill-delivered so the damage execution's BonusSkillDamage gate can see them.
    static void MarkSpecAsSkill(FMythicDamageContainerSpec &Spec);

    // Returns the dash task when one was started, so the caller knows what the skill is waiting on.
    UAbilityTask_ApplyRootMotionMoveToForce *StartMovement();

    UFUNCTION()
    void OnImpact(FGameplayEventData Payload);

    UFUNCTION()
    void OnSkillFinished();

private:
    const class UMythicAttributeSet_Offense *GetOffenseSet() const;

    float GetSkillDurationBonus() const;
};
