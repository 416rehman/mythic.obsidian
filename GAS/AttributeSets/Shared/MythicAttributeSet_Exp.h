#pragma once
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Exp.generated.h"

/// Player Progression ///
/// Overall player progression through the game.
UCLASS()
class MYTHIC_API UMythicAttributeSet_Exp : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", ReplicatedUsing = OnRep_XP)
    FGameplayAttributeData XP;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", ReplicatedUsing = OnRep_Level)
    FGameplayAttributeData Level;

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Exp, XP);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Exp, Level);

    UFUNCTION()
    virtual void OnRep_XP(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_Level(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    // SERVER: the canonical, GameplayEffect-free way to grant XP. Adds XP and processes any level-ups.
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void AddExperience(float Amount);

protected:
    // Processes level-ups from the current XP against the XPRequiredToLevelUp curve (shared by AddExperience
    // and the GameplayEffect XP path). Caps at the curve's max level and guards against zero-cost loops.
    void ProcessLevelUps();

    // Player-facing level-up feedback: a floating "Level Up!" over the LOCAL player's pawn. Called from BOTH the
    // authority path (ProcessLevelUps — covers a listen-server host) and the client path (OnRep_Level — remote clients).
    // IsLocallyControlled-guarded so each player sees only their OWN level-up, exactly once (Level replicates COND_None,
    // so OnRep also fires for other players' level-ups — those are correctly skipped). No-op off the local player.
    void HandleLevelUp(int32 OldLevel, int32 NewLevel);

    // CLIENT-side latch: the FIRST OnRep_Level is the initial replicated state (join/possession), not a level-up — its
    // OldValue is the default 0, which would spuriously fire "Level Up! Lv N". Set true after the first OnRep so only
    // subsequent increases celebrate. (The authority/ProcessLevelUps path is already gated on a real LevelUps>0.)
    bool bClientLevelInitialized = false;
};
