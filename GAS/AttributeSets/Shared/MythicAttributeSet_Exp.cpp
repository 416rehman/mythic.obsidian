#include "MythicAttributeSet_Exp.h"

#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Mythic.h"
#include "GameModes/GameState/MythicGameState.h"
#include "MythicAttributeSet_Utility.h"
#include "MythicAttributeSet_Life.h"
#include "MythicAttributeSet_Defense.h"
#include "GAS/MythicTags_GAS.h"
#include "Net/UnrealNetwork.h"
#include "Curves/RealCurve.h"
#include "UI/MythicDamageNumberSubsystem.h" // floating "Level Up!" callout
#include "GameFramework/Pawn.h" // IsLocallyControlled guard

void UMythicAttributeSet_Exp::OnRep_XP(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Exp, XP, OldValue);
}

void UMythicAttributeSet_Exp::OnRep_Level(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Exp, Level, OldValue);
    // Client path: the owning client celebrates its level-up when the replicated Level lands — EXCEPT the very first
    // replication (initial join/possession state), whose default-0 OldValue would spuriously fire "Level Up! Lv N".
    if (bClientLevelInitialized) {
        HandleLevelUp(static_cast<int32>(OldValue.GetCurrentValue()), static_cast<int32>(GetLevel()));
    }
    else {
        bClientLevelInitialized = true;
    }
}

void UMythicAttributeSet_Exp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Exp, XP, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Exp, Level, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Exp::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    // If XP was modified by a GameplayEffect, process any resulting level-ups.
    if (Data.EvaluatedData.Attribute == GetXPAttribute()) {
        ProcessLevelUps();
    }
}

void UMythicAttributeSet_Exp::AddExperience(float Amount) {
    if (Amount <= 0.0f) {
        return;
    }
    // Server-authoritative regardless of caller (this is BlueprintCallable).
    UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
    if (!ASC || !ASC->IsOwnerActorAuthoritative()) {
        return;
    }
    // Apply the %XP bonus (UMythicAttributeSet_Utility::ExperienceBonus — e.g. from gear). This is the single
    // canonical point EVERY XP source flows through (kills today; quests/gathering later), so the bonus accelerates
    // all leveling. Clamp the multiplier at 0 so a <= -100% bonus zeroes the gain rather than subtracting XP.
    if (const UMythicAttributeSet_Utility *Util = ASC->GetSet<UMythicAttributeSet_Utility>()) {
        Amount *= FMath::Max(0.0f, 1.0f + Util->GetExperienceBonus());
    }
    // GAS.Buff.Enlighten — a temporary enlightenment buff grants bonus XP (its declared meaning; it was never applied
    // before). Tunable scalar on the GameState, mirroring the status-damage scalar block; resolved via the same path
    // ProcessLevelUps uses. Stacks multiplicatively on top of the gear ExperienceBonus.
    if (ASC->HasMatchingGameplayTag(GAS_BUFF_ENLIGHTEN)) {
        if (const AMythicGameState *GS = GetWorld() ? GetWorld()->GetGameState<AMythicGameState>() : nullptr) {
            Amount *= FMath::Max(0.0f, 1.0f + GS->EnlightenXpBonus);
        }
    }
    if (Amount <= 0.0f) {
        return; // the bonus zeroed the gain
    }
    SetXP(GetXP() + Amount);
    ProcessLevelUps();
}

void UMythicAttributeSet_Exp::ProcessLevelUps() {
    AMythicGameState *GameState = GetWorld() ? GetWorld()->GetGameState<AMythicGameState>() : nullptr;
    if (!GameState) {
        return;
    }
    // XPLevelsCurve.XPRequiredToLevelUp: per-level XP required to reach the next level.
    FRealCurve *XPRequiredCurve = GameState->XPLevelsCurveRowHandle.GetCurve("");
    if (!XPRequiredCurve) {
        UE_LOG(Myth, Error, TEXT("MythicAttributeSet_Exp: XPLevelsCurveRowHandle is unset; cannot process level-ups."));
        return;
    }

    const int32 CurrentLevel = GetLevel();
    const int32 MaxLevel = XPRequiredCurve->GetNumKeys();

    // Already at max level: clamp XP to the cap and stop.
    if (CurrentLevel >= MaxLevel) {
        SetXP(XPRequiredCurve->Eval(CurrentLevel));
        return;
    }

    float CurrentXP = GetXP();
    float XPRequired = XPRequiredCurve->Eval(CurrentLevel);
    int32 LevelUps = 0;

    // Consume XP for each level-up; cap at max level and guard against a zero/negative cost (infinite loop).
    while (CurrentXP >= XPRequired && XPRequired > 0.0f && (CurrentLevel + LevelUps) < MaxLevel) {
        CurrentXP -= XPRequired;
        LevelUps++;
        XPRequired = XPRequiredCurve->Eval(CurrentLevel + LevelUps);
    }

    if (LevelUps > 0) {
        SetLevel(CurrentLevel + LevelUps);
        SetXP(CurrentXP);
        UE_LOG(Myth, Log, TEXT("MythicAttributeSet_Exp: leveled up %d -> %d (XP remainder %.1f)"), CurrentLevel, CurrentLevel + LevelUps, CurrentXP);

        // Authority path: a listen-server HOST celebrates here (its own attribute won't OnRep to itself). On a
        // dedicated server this no-ops (no locally-controlled avatar). Remote clients celebrate via OnRep_Level instead.
        HandleLevelUp(CurrentLevel, CurrentLevel + LevelUps);

        // Level-up rewards (server-authoritative — ProcessLevelUps runs only from the auth-gated AddExperience /
        // PostGameplayEffectExecute — so the OnRep_* notifies replicate the growth to clients). One ASC fetch for all.
        if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent()) {
            // (1) MaxHealth pool + immediate heal, via the Life set. Without this, leveling up changed only a number.
            const float HealthGain = GameState->HealthPerLevel * static_cast<float>(LevelUps);
            if (HealthGain > 0.0f) {
                if (const UMythicAttributeSet_Life *Life = ASC->GetSet<UMythicAttributeSet_Life>()) {
                    // The bigger pool always applies (a dead owner gets it on respawn via ResetForRespawn -> Health=Max).
                    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), Life->GetMaxHealth() + HealthGain);
                    // The HEAL only to a LIVING owner — don't raise a corpse's Health behind the latched DEAD tag (the
                    // sibling heals ApplyRegen/HandleDamageDelivered/HandleKill all dead-gate the same way).
                    if (!ASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
                        ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), Life->GetHealth() + HealthGain);
                    }
                }
            }
            // (2) Armor — flat damage mitigation consumed every hit by the damage execution (MythicDamageApplication
            // captures Defense::Armor) — via the Defense set. No dead-gate: Armor is a passive stat with no live/dead
            // semantics. Completes the progression loop: kill -> XP -> level -> tougher (tankier, not just bigger pool).
            const float ArmorGain = GameState->ArmorPerLevel * static_cast<float>(LevelUps);
            if (ArmorGain > 0.0f) {
                if (const UMythicAttributeSet_Defense *Defense = ASC->GetSet<UMythicAttributeSet_Defense>()) {
                    ASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetArmorAttribute(), Defense->GetArmor() + ArmorGain);
                }
            }
        }
    }
}

void UMythicAttributeSet_Exp::HandleLevelUp(int32 OldLevel, int32 NewLevel) {
    if (NewLevel <= OldLevel) {
        return; // only celebrate real increases (OnRep can fire on any Level change)
    }
    // The damage-number subsystem renders on the LOCAL HUD, so the callout only makes sense — and only fires — for the
    // locally-controlled avatar. This guard also makes the call cheap on a dedicated server and correctly skips other
    // players' level-ups (Level replicates COND_None, so OnRep fires for them too on this client).
    UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
    const APawn *Avatar = ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr;
    if (!Avatar || !Avatar->IsLocallyControlled()) {
        return;
    }
    UWorld *World = Avatar->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = Avatar->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
        const FString Text = FString::Printf(TEXT("Level Up!  Lv %d"), NewLevel);
        DamageNumbers->AddDamageNumberCustom(Location, Text, FLinearColor(1.0f, 0.85f, 0.1f), 2.0f); // gold, lingers 2s
    }
}
