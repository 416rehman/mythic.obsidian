
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "MythicGA_Skill.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;
class UAnimMontage;
class UMythicSkillComponent;

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
 * What the modifiers a character has switched on add up to. A skill can carry several at once, so the ability folds
 * one set of numbers rather than walking the list at every resolve point.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSkillModifierTotals {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    float RadiusDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    int32 TargetCountDelta = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    float DurationDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    float MovementDistanceDelta = 0.0f;

    // Two modifiers naming a status is a choice between them, not a sum, so the later one wins.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    FGameplayTag StatusOverride;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Skill")
    float StatusChanceDelta = 0.0f;

    /**
     * What the modifiers at ActiveIndices change, summed. An index the definition no longer has contributes nothing,
     * so content that dropped a modifier cannot make a saved character's build read past the end of the list.
     */
    static FMythicSkillModifierTotals Sum(const TArray<FMythicSkillModifier> &Modifiers, const TArray<int32> &ActiveIndices);
};

/**
 * Active skill. One class backs all sixteen: what a skill does is the shape it queries, the damage container it
 * applies, the status it inflicts and the way it moves you, all authored on a Blueprint child that carries no
 * graph.
 *
 * Three quantifiers take a bonus attribute off the caster - Radius, MaxTargets and the self-effect duration - so
 * gear moves what a skill does without the skill being re-authored. The modifiers the caster has bought on this
 * skill move those three as well, and movement distance, status chance and which status lands with them. A modifier
 * folds in beside the attribute, never instead of it: authored, then the modifiers, then the attribute, then the
 * clamp. Shape angle and forward offset are authored-only; neither an attribute nor a modifier reaches them yet.
 *
 * Damage is deliberately not scaled here. BonusSkillDamage is applied inside the damage execution to any hit tagged
 * GAS.Ability.Type.Skill, which this ability marks its own damage with, so scaling the container here would pay the
 * stat twice.
 *
 * A skill levels by being cast. One committed activation is one use, recorded on the authority copy only, and the
 * skill component decides what that use is worth. Nothing a client sends raises a level.
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

    // What the caster's active modifiers on this skill add up to. Empty for a caster with no skill component.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    FMythicSkillModifierTotals GetModifierTotals() const;

    // The authored shape with the active modifiers and the caster's bonus attributes folded in. What the query uses.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    FMythicSkillShape ResolveShape() const;

    // SelfEffectDuration with the active modifiers and SkillDurationBonus folded in.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    float ResolveSelfEffectDuration() const;

    // MovementDistance with the active modifiers folded in. No attribute moves it yet.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    float ResolveMovementDistance() const;

    // The Status.Type.* this skill inflicts once an active modifier has had its say.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    FGameplayTag ResolveStatusToApply() const;

    // StatusChance with the active modifiers folded in. No attribute moves it yet.
    UFUNCTION(BlueprintPure, Category = "Mythic|Skill")
    float ResolveStatusChance() const;

    // Never negative: a cursed radius shrinks a skill, it does not turn it inside out.
    static float ScaleRadius(float Authored, float Bonus, float ModifierDelta = 0.0f);

    /**
     * An uncapped skill (0) stays uncapped: only the authored number says whether the skill has a cap at all, so a
     * modifier that takes targets away caps a skill harder rather than uncapping it. A capped one never falls below
     * a single target.
     */
    static int32 ScaleTargetCount(int32 Authored, float Bonus, int32 ModifierDelta = 0);

    static float ScaleDuration(float Authored, float Bonus, float ModifierDelta = 0.0f);

    // An authored 0 means the skill does not move you, so no modifier can make it dash.
    static float ScaleMovementDistance(float Authored, float ModifierDelta);

    // Stays a probability: stacked chance modifiers can reach certainty, never pass it.
    static float ScaleStatusChance(float Authored, float ModifierDelta);

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

    void ApplyStatus(const TArray<FHitResult> &Hits, AActor *Instigator, const FGameplayTag &Status, float Chance) const;

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

    // The definition this ability was granted from. It is the spec's source object, so a skill fired from a slot
    // knows which set of authored modifiers is its own.
    UMythicSkillDefinition *GetSkillDefinition() const;

    UMythicSkillComponent *GetSkillComponent() const;
};
