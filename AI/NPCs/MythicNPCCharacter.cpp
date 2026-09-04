

#include "MythicNPCCharacter.h"

#include "MythicNPCManager.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "Settings/MythicCombatSettings.h"
#include "Settings/MythicAgentDetailSettings.h"
#include "AI/MythicTags_AI.h"
#include "GAS/MythicTags_GAS.h"
#include "AI/MonsterAffixes/MonsterAffixPool.h"
#include "AI/MonsterAffixes/MonsterAffixTypes.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "GameplayEffect.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"
#include "MassEntitySubsystem.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/NPCs/MythicNPCAIController.h"
#include "AI/Party/PartySubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Appearance/AppearanceTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Factions/FactionColor.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "Objectives/ObjectiveTracker.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/Entity/MythicEntityPresentationTags.h"
#include "EngineUtils.h"


const FMythicNPCData AMythicNPCCharacter::GetNPCData() const {
    return this->NPCData;
}

void AMythicNPCCharacter::SetEngagedTarget(AActor *Target) {
    if (!HasAuthority()) {
        return;
    }

    EngagedTarget = IsValid(Target) ? Target : nullptr;
    if (!EntityPresentationComponent) {
        return;
    }

    if (!EngagedTarget) {
        EntityPresentationComponent->ClearObservableFact(
            MythicEntityPresentationTags::ObservableSlotBehavior);
        return;
    }

    EntityPresentationComponent->SetObservableFact(
        MythicEntityPresentationTags::ObservableSlotBehavior,
        MythicEntityPresentationTags::ObservableBehaviorFighting,
        FMythicPresentationHandle());
}

void AMythicNPCCharacter::SetFleeingPresentation(const bool bIsFleeing) {
    if (!HasAuthority() || !EntityPresentationComponent) {
        return;
    }
    if (!EngagedTarget) {
        EntityPresentationComponent->ClearObservableFact(
            MythicEntityPresentationTags::ObservableSlotBehavior);
        return;
    }

    EntityPresentationComponent->SetObservableFact(
        MythicEntityPresentationTags::ObservableSlotBehavior,
        bIsFleeing
            ? MythicEntityPresentationTags::ObservableBehaviorFleeing
            : MythicEntityPresentationTags::ObservableBehaviorFighting,
        FMythicPresentationHandle());
}

bool AMythicNPCCharacter::OnSpawnedFromPool(const struct FMythicNPCData &InNPCData) {
    if (!this->HasAuthority()) {
        return false;
    }
    // The body remains invisible and unregistered until every field for its next logical person is complete.
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    this->NPCData = InNPCData;
    ParkMovementForPool();

    // A recycled actor was unpossessed on pool return; a fresh SpawnActor already auto-possessed.
    if (!GetController()) {
        if (AController *Retained = PooledController.Get()) {
            Retained->Possess(this);
        }
        else {
            SpawnDefaultController();
        }
    }
    PooledController = nullptr;

    this->InitializeASC();

    InitializeBrainFromNPCData();

    if (EntityPresentationComponent) {
        FMythicPublicIdentitySnapshot SafeIdentity;
        SafeIdentity.PublicKindTag =
            MythicEntityPresentationTags::EntityKindHumanoid;
        SafeIdentity.PublicIdentityDefinitionId =
            UMythicEntityIdentityDefinition::ResolvePrimaryAssetId(
                NPCData.PublicIdentityDefinition);
        EntityPresentationComponent->AuthorityPrepareEmbodiment(
            NPCData.EntityId, SafeIdentity);
    }

    const bool bActivated = ActivatePreparedEmbodiment();
    if (!bActivated) {
        UE_LOG(Myth, Error,
               TEXT("AMythicNPCCharacter::OnSpawnedFromPool left %s parked because its authoritative combat commit failed."),
               *GetNameSafe(this));
    }
    return bActivated;
}

void AMythicNPCCharacter::ParkMovementForPool() {
    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        CMC->StopMovementImmediately();
        CMC->DisableMovement();
        CMC->SetComponentTickEnabled(false);
    }
}

void AMythicNPCCharacter::RestoreMovementFromPool() {
    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        CMC->SetComponentTickEnabled(true);
        CMC->StopMovementImmediately();
        CMC->SetMovementMode(MOVE_Walking);
    }
}

void AMythicNPCCharacter::InitializeBrainFromNPCData() {
    if (!CognitiveBrain || !NPCData.Faction.IsValid()) {
        return;
    }
    const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicFactionDatabase *FactionDB = LWS ? LWS->GetFactionDatabase() : nullptr;
    if (!FactionDB) {
        return;
    }
    const FMythicFactionId FactionId = FactionDB->FindFactionId(NPCData.Faction);
    if (!FactionId.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("InitializeBrainFromNPCData: %s carries faction %s which the faction database does not know."),
               *GetNameSafe(this), *NPCData.Faction.ToString());
        return;
    }

    // The per-faction fight map collapses to scalar vent weights until a per-faction threat consumer
    // exists; the full map stays authored on NPCData for that day.
    FMythicPersonalityFragment Personality;
    if (NPCData.FlightOrFightOverrides.Num() > 0) {
        float Sum = 0.0f;
        for (const TPair<FGameplayTag, float> &Pair : NPCData.FlightOrFightOverrides) {
            Sum += Pair.Value;
        }
        const float Fight = FMath::Clamp(Sum / NPCData.FlightOrFightOverrides.Num(), 0.0f, 1.0f);
        Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] = Fight;
        Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] = 1.0f - Fight;
    }

    FMythicCellCoord HomeCell;
    if (const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
        HomeCell = Grid->WorldToCell(GetActorLocation());
    }

    CognitiveBrain->InitializeBrain(FactionId, HomeCell, Personality, FMassEntityHandle());
}

void AMythicNPCCharacter::GrantAttackAbility() {
    if (!HasAuthority() || !AbilitySystemComponent || !AttackAbility) {
        return;
    }
    if (AttackAbilityHandle.IsValid()) {
        return;
    }
    AttackAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(AttackAbility.GetDefaultObject(), 1, INDEX_NONE, this));
}

void AMythicNPCCharacter::CapturePristineAttributeBases() {
    if (!HasAuthority() || !AbilitySystemComponent
        || bPristineAttributeBasesCaptured) {
        return;
    }

    TArray<FGameplayAttribute> Attributes;
    AbilitySystemComponent->GetAllAttributes(Attributes);
    PristineAttributeBases.Empty(Attributes.Num());
    for (const FGameplayAttribute &Attribute : Attributes) {
        if (!Attribute.IsValid()
            || !AbilitySystemComponent->HasAttributeSetForAttribute(Attribute)) {
            continue;
        }
        const float Base =
            AbilitySystemComponent->GetNumericAttributeBase(Attribute);
        if (FMath::IsFinite(Base)) {
            PristineAttributeBases.Add(Attribute, Base);
        }
    }
    bPristineAttributeBasesCaptured = !PristineAttributeBases.IsEmpty();
    if (!bPristineAttributeBasesCaptured) {
        UE_LOG(Myth, Error,
               TEXT("AMythicNPCCharacter::CapturePristineAttributeBases found no GAS attributes on %s."),
               *GetNameSafe(this));
    }
}

void AMythicNPCCharacter::RestorePristineAttributeBases() {
    if (!HasAuthority() || !AbilitySystemComponent
        || !bPristineAttributeBasesCaptured) {
        return;
    }
    for (const TPair<FGameplayAttribute, float> &Pair :
         PristineAttributeBases) {
        if (Pair.Key.IsValid()
            && AbilitySystemComponent->HasAttributeSetForAttribute(Pair.Key)
            && FMath::IsFinite(Pair.Value)) {
            AbilitySystemComponent->SetNumericAttributeBase(Pair.Key,
                                                            Pair.Value);
        }
    }
}

void AMythicNPCCharacter::ResetCombatRuntimeStateToPristine() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    AbilitySystemComponent->CancelAllAbilities();
    FMonsterAffixGranter::RemoveMonsterAffixes(AbilitySystemComponent,
                                               MonsterAffixHandles);
    AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
    AbilitySystemComponent->ClearAllAbilities();
    AbilitySystemComponent->RemoveAllGameplayCues();

    FGameplayTagContainer OwnedTags;
    AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
    for (const FGameplayTag &Tag : OwnedTags) {
        AbilitySystemComponent->SetLooseGameplayTagCount(Tag, 0);
    }

    DefaultEffectHandles.Reset();
    CombatScalingHandle.Invalidate();
    MonsterAffixHandles.Reset();
    AttackAbilityHandle = FGameplayAbilitySpecHandle();
    RestorePristineAttributeBases();
    bCombatInitialized = false;
}

bool AMythicNPCCharacter::HasCanonicalCombatAttributeSets() const {
    return AbilitySystemComponent
        && AbilitySystemComponent->GetSet<UMythicAttributeSet_Life>()
        && AbilitySystemComponent->GetSet<UMythicAttributeSet_Offense>()
        && AbilitySystemComponent->GetSet<UMythicAttributeSet_Defense>()
        && AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>();
}

bool AMythicNPCCharacter::CommitCombatInitializationForEmbodiment() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return false;
    }
    if (bCombatInitialized) {
        return true;
    }

    InitializeASC();
    if (!bPristineAttributeBasesCaptured || !HasCanonicalCombatAttributeSets()) {
        UE_LOG(Myth, Error,
               TEXT("Combat commit rejected %s: every combat entity requires canonical Life, Offense, Defense, and Utility AttributeSets."),
               *GetNameSafe(this));
        return false;
    }

    ResetCombatRuntimeStateToPristine();

    auto RollBackCommit = [this]() -> bool {
        ResetCombatRuntimeStateToPristine();
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        SetActorTickEnabled(false);
        ParkMovementForPool();
        return false;
    };

    auto ApplyAuthoredEffect = [this](
                                   const TSubclassOf<UGameplayEffect> &Effect,
                                   const bool bExpectActiveHandle) -> bool {
        if (!Effect) {
            UE_LOG(Myth, Error,
                   TEXT("Combat commit rejected %s: DefaultGameplayEffects contains a null entry."),
                   *GetNameSafe(this));
            return false;
        }
        FGameplayEffectContextHandle Context =
            AbilitySystemComponent->MakeEffectContext();
        Context.AddSourceObject(this);
        const FGameplayEffectSpecHandle Spec =
            AbilitySystemComponent->MakeOutgoingSpec(Effect, 1.0f, Context);
        if (!Spec.IsValid()) {
            UE_LOG(Myth, Error,
                   TEXT("Combat commit rejected %s: could not construct %s."),
                   *GetNameSafe(this), *GetNameSafe(Effect.Get()));
            return false;
        }
        const FActiveGameplayEffectHandle Applied =
            AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
                *Spec.Data.Get());
        if (bExpectActiveHandle && !Applied.IsValid()) {
            UE_LOG(Myth, Error,
                   TEXT("Combat commit rejected %s: persistent effect %s did not produce an active handle."),
                   *GetNameSafe(this), *GetNameSafe(Effect.Get()));
            return false;
        }
        if (Applied.IsValid()) {
            DefaultEffectHandles.Add(Applied);
        }
        return true;
    };

    // Authoritative order is binding: base values first, rolled proficiencies second, then all derived layers.
    for (const TSubclassOf<UGameplayEffect> &Effect :
         DefaultGameplayEffects) {
        const UGameplayEffect *EffectCDO = Effect.GetDefaultObject();
        if (!EffectCDO
            || EffectCDO->DurationPolicy
                   != EGameplayEffectDurationType::Instant) {
            continue;
        }
        if (!ApplyAuthoredEffect(Effect, false)) {
            return RollBackCommit();
        }
    }

    for (const FRolledAttributeSpec &Roll : NPCData.Proficiencies) {
        if (!Roll.Attribute.IsValid()
            || !AbilitySystemComponent->HasAttributeSetForAttribute(
                Roll.Attribute)) {
            continue;
        }
        if (const float *Pristine =
                PristineAttributeBases.Find(Roll.Attribute)) {
            const float Baseline =
                AbilitySystemComponent->GetNumericAttributeBase(
                    Roll.Attribute);
            if (!FMath::IsNearlyEqual(Baseline, *Pristine)) {
                UE_LOG(Myth, Warning,
                       TEXT("%s composes both an authored instant baseline (%.2f) and an NPC proficiency on %s; verify that this explicit composition is intended."),
                       *GetNameSafe(this), Baseline,
                       *Roll.Attribute.GetName());
            }
        }
    }
    SeedAttributesFromData();

    for (const TSubclassOf<UGameplayEffect> &Effect :
         DefaultGameplayEffects) {
        const UGameplayEffect *EffectCDO = Effect.GetDefaultObject();
        if (!EffectCDO) {
            return RollBackCommit();
        }
        if (EffectCDO->DurationPolicy
            == EGameplayEffectDurationType::Instant) {
            continue;
        }
        if (!ApplyAuthoredEffect(Effect, true)) {
            return RollBackCommit();
        }
    }

    GrantAttackAbility();
    ApplyCombatScaling(false);
    if (!CombatScalingHandle.IsValid()) {
        UE_LOG(Myth, Error,
               TEXT("Combat commit rejected %s: canonical combat scaling failed to apply."),
               *GetNameSafe(this));
        return RollBackCommit();
    }

    PublishIdentityTags();
    if (!NPCData.Traits.IsEmpty()) {
        AbilitySystemComponent->AddLooseGameplayTags(NPCData.Traits);
    }
    ApplyMonsterAffixes();

    const FGameplayAttribute MaxHealthAttribute =
        UMythicAttributeSet_Life::GetMaxHealthAttribute();
    const FGameplayAttribute HealthAttribute =
        UMythicAttributeSet_Life::GetHealthAttribute();
    const float MaximumHealth =
        AbilitySystemComponent->GetNumericAttribute(MaxHealthAttribute);
    // MaxHealth is assigned by the authored baseline, derivation and scaling layers above. The AttributeSet's
    // one-point corruption floor is not an NPC lifecycle sentinel: rejecting an actor because evaluation reached
    // that floor only hides the calculation defect and repeatedly re-runs it on the next pool attempt.
    if (!FMath::IsFinite(MaximumHealth) || MaximumHealth <= 0.0f) {
        UE_LOG(Myth, Error,
               TEXT("Combat commit rejected %s: the completed authored vitality pipeline produced non-positive or non-finite MaxHealth %.3f."),
               *GetNameSafe(this), MaximumHealth);
        return RollBackCommit();
    }

    if (LifeAttributes) {
        LifeAttributes->ResetForRespawn();
    }
    AbilitySystemComponent->SetNumericAttributeBase(HealthAttribute,
                                                    MaximumHealth);
    if (LifeComponent) {
        LifeComponent->RestoreAfterDeath();
    }

    const float FinalHealth =
        AbilitySystemComponent->GetNumericAttribute(HealthAttribute);
    if (!FMath::IsFinite(FinalHealth) || FinalHealth <= 0.0f
        || !FMath::IsNearlyEqual(FinalHealth, MaximumHealth, 0.01f)) {
        UE_LOG(Myth, Error,
               TEXT("Combat commit rejected %s: final vitality is %.3f / %.3f."),
               *GetNameSafe(this), FinalHealth, MaximumHealth);
        return RollBackCommit();
    }

    bCombatInitialized = true;
    return true;
}

void AMythicNPCCharacter::ApplyMonsterAffixes() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    if (!MonsterAffixHandles.IsEmpty()) {
        FMonsterAffixGranter::RemoveMonsterAffixes(AbilitySystemComponent, MonsterAffixHandles);
    }

    const int32 EnemyTierInt = GetAITierInt(EnemyTier);
    if (EnemyTierInt <= 1) {
        return;
    }

    uint8 Danger = static_cast<uint8>(EMythicDangerTier::Safe);
    if (const UGameInstance *GI = GetGameInstance()) {
        if (const UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            if (const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid()) {
                Danger = static_cast<uint8>(Grid->GetCellDangerTier(Grid->WorldToCell(GetActorLocation())));
            }
        }
    }

    const int32 Budget = FMonsterAffixSelector::ComputeAffixBudget(EnemyTierInt, Danger);
    if (Budget <= 0) {
        return;
    }

    const TArray<FMonsterAffixDef> &Pool =
        (MonsterAffixPool && MonsterAffixPool->Defs.Num() > 0) ? MonsterAffixPool->Defs : UMonsterAffixPool::GetDefaultPool();

    const FVector SnappedLoc = GetActorLocation().GridSnap(100.0f);
    const int32 LocHash = static_cast<int32>(FMath::Fmod(FMath::Abs(SnappedLoc.X), 1.0e6))
                        ^ (static_cast<int32>(FMath::Fmod(FMath::Abs(SnappedLoc.Y), 1.0e6)) << 8);
    FRandomStream Rng(static_cast<int32>(GetTypeHash(GetFName())) ^ LocHash
                      ^ (EnemyTierInt << 16) ^ (static_cast<int32>(Danger) << 24));

    const TArray<FGameplayTag> Selected = FMonsterAffixSelector::Select(EnemyTierInt, Danger, Budget, Pool, Rng);
    if (Selected.Num() == 0) {
        return;
    }

    MonsterAffixHandles = FMonsterAffixGranter::GrantMonsterAffixes(AbilitySystemComponent, Selected, MonsterAffixPool);
    UE_LOG(Myth, Verbose, TEXT("AMythicNPCCharacter::ApplyMonsterAffixes: %s granted %d affix(es) (tier %d, danger %u, budget %d)"),
           *GetNameSafe(this), Selected.Num(), EnemyTierInt, Danger, Budget);
}

void AMythicNPCCharacter::PublishIdentityTags() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    auto PublishOnce = [this](const FGameplayTag &Tag) {
        if (Tag.IsValid() && !AbilitySystemComponent->HasMatchingGameplayTag(Tag)) {
            AbilitySystemComponent->AddLooseGameplayTag(Tag);
        }
    };

    PublishOnce(CreatureKind);
    PublishOnce(EnemyTier);

    // CodexBestiaryKey is the explicit override and the mapper reads it first. Without the type published too,
    // there was nothing to derive from when no override was authored - which was every NPC, since the key is
    // assigned nowhere - so every humanoid kill credited the generic page.
    PublishOnce(NPCData.NPCType);
    PublishOnce(CodexBestiaryKey);
}

float AMythicNPCCharacter::ResolveStableScalingPercentile(
    const uint32 Salt) const {
    FMythicEntityId StableEntityId = NPCData.EntityId;
    if (!StableEntityId.IsValid() && EntityPresentationComponent) {
        StableEntityId =
            EntityPresentationComponent->GetAuthorityEntityId();
    }
    if (!StableEntityId.IsValid()) {
        UE_LOG(Myth, Error,
               TEXT("%s has no typed logical entity identity for deterministic combat scaling; using the neutral percentile."),
               *GetNameSafe(this));
        return 0.5f;
    }

    const uint32 Seed =
        HashCombineFast(GetTypeHash(StableEntityId), Salt);
    FRandomStream Stream(static_cast<int32>(Seed));
    return Stream.FRand();
}

void AMythicNPCCharacter::ApplyCombatScaling(
    const bool bPreserveHealthRatio) {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    const FGameplayAttribute HealthAttribute =
        UMythicAttributeSet_Life::GetHealthAttribute();
    const FGameplayAttribute MaxHealthAttribute =
        UMythicAttributeSet_Life::GetMaxHealthAttribute();
    const float PreviousHealth =
        AbilitySystemComponent->GetNumericAttribute(HealthAttribute);
    const float PreviousMaximumHealth =
        AbilitySystemComponent->GetNumericAttribute(MaxHealthAttribute);
    const bool bRestoreHealthRatio = bPreserveHealthRatio
        && bCombatInitialized && FMath::IsFinite(PreviousHealth)
        && FMath::IsFinite(PreviousMaximumHealth)
        && PreviousMaximumHealth > UE_SMALL_NUMBER;
    const bool bWasDead = bRestoreHealthRatio && PreviousHealth <= 0.0f;
    const float PreviousHealthRatio = bRestoreHealthRatio
        ? FMath::Clamp(PreviousHealth / PreviousMaximumHealth, 0.0f, 1.0f)
        : 1.0f;

    if (EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityBeginAbilitySystemProjectionBatch();
    }

    int32 PartySize = 1;
    float WorldHealthMult = 1.0f;
    float WorldDamageMult = 1.0f;
    if (const UWorld *World = GetWorld()) {
        if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
            PartySize = GS->PlayerArray.Num();
            if (const UWorldTierAttributes *WTA = GS->WorldTierAttributes) {
                WorldHealthMult = WTA->GetEnemyHealthMultiplier();
                WorldDamageMult = WTA->GetEnemyDamageMultiplier();
            }
        }
    }

    const FVector2D PartyWorldMult = FMythicEnemyScaling::ComputeStatMultiplier(
        PartySize, PerExtraMemberHealth, PerExtraMemberDamage, WorldHealthMult, WorldDamageMult);
    const FMythicTierScaling Tier = FMythicEnemyScaling::GetTierScaling(EnemyTier);

    // The level half: sample a stable percentile of the GameState's min/max curves. The percentile belongs to the
    // typed logical entity, so a live tier/party update or a different pooled body never rerolls that NPC.
    //
    // Combatants only. A living world spawns merchants, farmers and wanderers from these same pools, and their
    // disposition is flee-or-talk, not fight - an authored attack ability is what marks an entity as combat-trained, so a
    // civilian keeps its authored stats no matter how dangerous its home territory is. CombatLevel
    // still stamps on everyone: it is world context (loot, trade stock, XP), not a threat statement.
    float LevelHealthMult = 1.0f;
    float LevelDamageMult = 1.0f;
    if (const UWorld *World = AttackAbility ? GetWorld() : nullptr) {
        if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
            if (!bScalingPercentilesInitialized) {
                HealthLevelPercentile =
                    ResolveStableScalingPercentile(0x48EA17u);
                DamageLevelPercentile =
                    ResolveStableScalingPercentile(0xD4A6E3u);
                bScalingPercentilesInitialized = true;
            }
            const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
            const float Level = static_cast<float>(FMath::Max(1, NPCData.CombatLevel));
            const float HealthTailGrowth =
                Settings ? Settings->CombatantHealthTailGrowth : 0.0f;
            const float DamageTailGrowth =
                Settings ? Settings->CombatantDamageTailGrowth : 0.0f;
            const float HealthLow = MythicCombat::SampleOpenEnded(GS->HealthMinCurveRowHandle, Level, HealthTailGrowth);
            const float HealthHigh = MythicCombat::SampleOpenEnded(GS->HealthMaxCurveRowHandle, Level, HealthTailGrowth);
            const float DamageLow = MythicCombat::SampleOpenEnded(GS->DamageMinCurveRowHandle, Level, DamageTailGrowth);
            const float DamageHigh = MythicCombat::SampleOpenEnded(GS->DamageMaxCurveRowHandle, Level, DamageTailGrowth);
            LevelHealthMult = FMath::Lerp(
                FMath::Min(HealthLow, HealthHigh),
                FMath::Max(HealthLow, HealthHigh),
                HealthLevelPercentile);
            LevelDamageMult = FMath::Lerp(
                FMath::Min(DamageLow, DamageHigh),
                FMath::Max(DamageLow, DamageHigh),
                DamageLevelPercentile);
        }
    }

    const float UnvalidatedHealthMult =
        static_cast<float>(PartyWorldMult.X) * Tier.HealthMult
        * LevelHealthMult;
    const float UnvalidatedDamageMult =
        static_cast<float>(PartyWorldMult.Y) * Tier.DamageMult
        * LevelDamageMult;
    const float HealthMult =
        FMath::IsFinite(UnvalidatedHealthMult)
            && UnvalidatedHealthMult > 0.0f
        ? UnvalidatedHealthMult
        : 1.0f;
    const float DamageMult =
        FMath::IsFinite(UnvalidatedDamageMult)
            && UnvalidatedDamageMult > 0.0f
        ? UnvalidatedDamageMult
        : 1.0f;

    if (CombatScalingHandle.IsValid()) {
        AbilitySystemComponent->RemoveActiveGameplayEffect(CombatScalingHandle);
        CombatScalingHandle.Invalidate();
    }
    FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
    Ctx.AddSourceObject(this);
    const FGameplayEffectSpecHandle Spec =
        AbilitySystemComponent->MakeOutgoingSpec(UMythicGE_CombatScaling::StaticClass(), 1.0f, Ctx);
    if (Spec.IsValid()) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_SCALING_HEALTH, HealthMult);
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_SCALING_DAMAGE, DamageMult);
        CombatScalingHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }

    if (bRestoreHealthRatio && CombatScalingHandle.IsValid()) {
        const float NewMaximumHealth =
            AbilitySystemComponent->GetNumericAttribute(MaxHealthAttribute);
        if (FMath::IsFinite(NewMaximumHealth)
            && NewMaximumHealth > UE_SMALL_NUMBER) {
            AbilitySystemComponent->SetNumericAttributeBase(
                HealthAttribute,
                bWasDead ? 0.0f : NewMaximumHealth * PreviousHealthRatio);
        }
    }

    if (LifeComponent) {
        if (!bBaseXPRewardCaptured) {
            BaseXPReward = LifeComponent->XPReward;
            bBaseXPRewardCaptured = true;
        }
        LifeComponent->XPReward = BaseXPReward * Tier.XpMult;
    }

    if (EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityEndAbilitySystemProjectionBatch();
    }
}

bool AMythicNPCCharacter::TryActivateAttack() {
    if (!HasAuthority() || !AbilitySystemComponent || !AttackAbilityHandle.IsValid()) {
        return false;
    }
    return AbilitySystemComponent->TryActivateAbility(AttackAbilityHandle);
}

void AMythicNPCCharacter::SeedAttributesFromData() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    for (FRolledAttributeSpec &Roll : NPCData.Proficiencies) {
        if (!Roll.Attribute.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AMythicNPCCharacter::SeedAttributesFromData: invalid attribute on %s; skipping spec."),
                   *GetNameSafe(this));
            continue;
        }

        if (!AbilitySystemComponent->HasAttributeSetForAttribute(Roll.Attribute)) {
            UE_LOG(Myth, Warning, TEXT("AMythicNPCCharacter::SeedAttributesFromData: %s has no attribute set for %s; skipping seed."),
                   *GetNameSafe(this), *Roll.Attribute.GetName());
            continue;
        }

        AbilitySystemComponent->ApplyModToAttribute(Roll.Attribute, Roll.Definition.Modifier, Roll.Value);
    }
}

void AMythicNPCCharacter::OnReturnedToPool() {
    if (!HasAuthority()) {
        return;
    }

    // Revoke presentation first while the old handle still resolves for owner-grant cleanup.
    if (EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
    }
    EngagedTarget = nullptr;
    CurrentActivityTag = FGameplayTag();
    Appearance = FMythicAppearance();

    if (LifeComponent) {
        LifeComponent->UninitializeFromAbilitySystem();
    }
    ResetCombatRuntimeStateToPristine();

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearAllTimersForObject(this);
        if (AController *AIController = GetController()) {
            World->GetTimerManager().ClearAllTimersForObject(AIController);
            // Retain the controller for the next wake; without this the recycled NPC comes back
            // controller-less and the old controller leaks in the world.
            PooledController = AIController;
            AIController->UnPossess();
        }
    }

    if (CognitiveBrain) {
        CognitiveBrain->StopThinking();
        CognitiveBrain->ResetForReuse();
    }

    // Pooled actors must not linger in the world: invisible, intangible, and parked until reuse.
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    ParkMovementForPool();

    bScalingPercentilesInitialized = false;
    HealthLevelPercentile = 0.5f;
    DamageLevelPercentile = 0.5f;
    NPCData.ClearAll();
}

void AMythicNPCCharacter::SleepToPool() {
    if (!HasAuthority()) {
        return;
    }

    OnReturnedToPool();
}

void AMythicNPCCharacter::WakeFromPool() {
    if (!HasAuthority()) {
        return;
    }

    InitializeASC();

    if (!GetController()) {
        if (AController *Retained = PooledController.Get()) {
            Retained->Possess(this);
        }
        else {
            SpawnDefaultController();
        }
    }
    PooledController = nullptr;

    ParkMovementForPool();
}

void AMythicNPCCharacter::PrepareForEmbodiment() {
    if (!HasAuthority()) {
        return;
    }
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    if (EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
    }
    EngagedTarget = nullptr;
    CurrentActivityTag = FGameplayTag();
    Appearance = FMythicAppearance();
    WakeFromPool();
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
}

bool AMythicNPCCharacter::ActivatePreparedEmbodiment() {
    if (!HasAuthority() || !EntityPresentationComponent
        || !EntityPresentationComponent->GetAuthorityEntityId().IsValid()) {
        return false;
    }
    if (!CommitCombatInitializationForEmbodiment()) {
        return false;
    }

    // Bind only after final attributes exist. The presentation component then publishes one coherent initial
    // vitality/status snapshot when the prepared embodiment becomes active.
    EntityPresentationComponent->AuthorityBindAbilitySystem(
        AbilitySystemComponent);
    if (!EntityPresentationComponent->AuthorityActivateEmbodiment()) {
        return false;
    }
    if (CognitiveBrain) {
        CognitiveBrain->StartThinking();
    }
    RestoreMovementFromPool();
    SetActorTickEnabled(PrimaryActorTick.bCanEverTick);
    SetActorEnableCollision(true);
    SetActorHiddenInGame(false);
    ForceNetUpdate();
    return true;
}

const FGuid &AMythicNPCCharacter::GetNPCId() const {
    return this->NPCData.NPCId;
}

const FGameplayTag &AMythicNPCCharacter::GetNPCType() const {
    return this->NPCData.NPCType;
}

AMythicNPCCharacter::AMythicNPCCharacter() {
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>("AbilitySystemComponent");
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>("LifeAttributes");
    DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>("DefenseAttributes");
    OffenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Offense>("OffenseAttributes");
    UtilityAttributes = CreateDefaultSubobject<UMythicAttributeSet_Utility>("UtilityAttributes");

    EnemyTier = AI_TIER_NORMAL;

    CreatureKind = AI_KIND_HUMANOID;

    CognitiveBrain = CreateDefaultSubobject<UMythicCognitiveBrainComponent>("CognitiveBrain");

    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>("LifeComponent");

    EntityPresentationComponent =
        CreateDefaultSubobject<UMythicEntityPresentationComponent>(
            TEXT("EntityPresentationComponent"));

    AIControllerClass = AMythicNPCAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // ACharacter defaults bUseControllerRotationYaw to true, so AAIController::Tick rotates the pawn through
    // APawn::FaceRotation. That write does not replay during movement re-simulation, and on any pawn whose Blueprint
    // also sets bOrientRotationToMovement the two fight every frame: FaceRotation snaps yaw to the control rotation
    // and PhysicsRotation turns it straight back toward velocity. Taking facing from the movement component instead
    // leaves one authority, inside PerformMovement.
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        // Blueprints that set bOrientRotationToMovement still win: PhysicsRotation checks that flag first.
        CMC->bUseControllerDesiredRotation = true;
    }

    if (UCapsuleComponent *Capsule = GetCapsuleComponent()) {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }

    if (USkeletalMeshComponent *MeshComp = GetMesh()) {
        MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        MeshComp->bEnableUpdateRateOptimizations = true;
    }
}


void AMythicNPCCharacter::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AController *C = Cast<AController>(Interactor);
    if (!C) {
        if (const APawn *P = Cast<APawn>(Interactor)) {
            C = P->GetController();
        }
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(C);
    if (!PC || !PC->IsLocalController()) {
        return;
    }

    if (IsRecruitable()) {
        PC->ServerRecruitNpc(this);
        return;
    }

    PC->ServerRequestNpcDialogue(this);
}

FText AMythicNPCCharacter::SelectDialogueFor(APlayerController *Interactor) const {
    return CognitiveBrain ? CognitiveBrain->SelectDialogue(Interactor) : FText::GetEmpty();
}

void AMythicNPCCharacter::FireBark(const FText &Line, APlayerController *Interactor) {
    OnNpcBark(Line, Interactor);
}

void AMythicNPCCharacter::OpenTradeForLocalController(
    APlayerController *Interactor) {
    if (IsMerchant() && Interactor && Interactor->IsLocalController()) {
        OnTradeOpened(Interactor);
    }
}

const UMythicContextActionDefinition *
AMythicNPCCharacter::ResolveContextActionDefinition(
    const FGameplayTag ActionTag) const {
    if (!ActionTag.IsValid()) {
        return nullptr;
    }
    const UMythicContextActionDefinition *Definitions[] = {
        QuestTurnInContextActionDefinition,
        QuestOfferContextActionDefinition,
        ServiceContextActionDefinition,
        TalkContextActionDefinition,
    };
    for (const UMythicContextActionDefinition *Definition : Definitions) {
        if (IsValid(Definition) && Definition->ActionTag == ActionTag) {
            return Definition;
        }
    }
    return nullptr;
}

bool AMythicNPCCharacter::IsContextActionAvailable(
    AController *RequestingController,
    const UMythicContextActionDefinition *Definition) const {
    const AMythicPlayerController *PlayerController =
        Cast<AMythicPlayerController>(RequestingController);
    if (!HasAuthority() || !PlayerController || !PlayerController->GetPawn()
        || !IsValid(Definition) || !Definition->ActionTag.IsValid()
        || (LifeComponent
            && (LifeComponent->IsDead() || LifeComponent->IsDowned()))
        || IsValid(EngagedTarget)) {
        return false;
    }

    if (Definition == TalkContextActionDefinition) {
        return CognitiveBrain != nullptr;
    }
    if (Definition == ServiceContextActionDefinition) {
        return IsMerchant();
    }

    const UObjectiveTracker *Tracker =
        PlayerController->GetObjectiveTracker();
    if (!Tracker) {
        return false;
    }
    if (Definition == QuestOfferContextActionDefinition) {
        FObjectiveProgress Progress;
        return QuestOffer
            && Tracker->EvaluateObjectiveOffer(QuestOffer, Progress)
                   == EObjectiveOfferResult::Assigned;
    }
    if (Definition == QuestTurnInContextActionDefinition) {
        return Tracker->CanAdvanceNpcInteraction(
            QuestNpcTag, PlayerController->GetInventoryComponent());
    }
    return false;
}

uint32 AMythicNPCCharacter::BuildContextActionRevision(
    const UMythicContextActionDefinition *Definition) const {
    uint32 Revision = Definition
        ? GetTypeHash(Definition->ActionTag) : 0;
    if (EntityPresentationComponent) {
        Revision = HashCombineFast(
            Revision,
            GetTypeHash(EntityPresentationComponent->GetPresentationInstance()));
    }
    return Revision == 0 ? 1 : Revision;
}

bool AMythicNPCCharacter::ValidateContextAction(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) const {
    OutFailureReason = FGameplayTag();
    const UMythicContextActionDefinition *Definition =
        ResolveContextActionDefinition(ActionTag);
    if (!HasAuthority() || Subject != this || !Definition
        || !RequestingController
        || RequestingController->GetWorld() != GetWorld()) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }
    if (ObservedOfferRevision < 0
        || ObservedOfferRevision > static_cast<int64>(MAX_uint32)
        || static_cast<uint32>(ObservedOfferRevision)
               != BuildContextActionRevision(Definition)) {
        OutFailureReason = CONTEXT_ACTION_REASON_STALE;
        return false;
    }
    if (!IsContextActionAvailable(RequestingController, Definition)) {
        OutFailureReason = CONTEXT_ACTION_REASON_UNAVAILABLE;
        return false;
    }
    return true;
}

void AMythicNPCCharacter::GatherContextActions_Implementation(
    AController *RequestingController, AActor *Subject,
    TArray<FMythicContextActionOffer> &OutOffers) const {
    if (!HasAuthority() || Subject != this || !RequestingController) {
        return;
    }

    const UMythicContextActionDefinition *Definitions[] = {
        QuestTurnInContextActionDefinition,
        QuestOfferContextActionDefinition,
        ServiceContextActionDefinition,
        TalkContextActionDefinition,
    };
    TArray<FGameplayTag, TInlineAllocator<4>> AddedActions;
    for (const UMythicContextActionDefinition *Definition : Definitions) {
        if (!IsContextActionAvailable(RequestingController, Definition)
            || AddedActions.Contains(Definition->ActionTag)) {
            continue;
        }
        FMythicContextActionOffer Offer;
        Offer.Definition = const_cast<UMythicContextActionDefinition *>(
            Definition);
        Offer.Availability = EMythicContextActionAvailability::Available;
        Offer.SourceRevision = static_cast<int64>(
            BuildContextActionRevision(Definition));
        OutOffers.Add(MoveTemp(Offer));
        AddedActions.Add(Definition->ActionTag);
    }
}

bool AMythicNPCCharacter::CanExecuteContextAction_Implementation(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) const {
    return ValidateContextAction(RequestingController, Subject, ActionTag,
                                 ObservedOfferRevision, OutFailureReason);
}

bool AMythicNPCCharacter::ExecuteContextAction_Implementation(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) {
    if (!ValidateContextAction(RequestingController, Subject, ActionTag,
                               ObservedOfferRevision, OutFailureReason)) {
        return false;
    }

    AMythicPlayerController *PlayerController =
        Cast<AMythicPlayerController>(RequestingController);
    const UMythicContextActionDefinition *Definition =
        ResolveContextActionDefinition(ActionTag);
    if (!PlayerController || !Definition) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }

    if (Definition == ServiceContextActionDefinition) {
        PlayerController->ClientOpenNpcTrade(this);
    } else {
        PlayerController->ServerRequestNpcDialogue(this);
    }
    OutFailureReason = FGameplayTag();
    return true;
}


FMythicSocialReactionResult AMythicNPCCharacter::ResolveSocialVerb(EMythicSocialVerb Verb, APlayerController *Interactor) const {
    FMythicSocialReactionResult Result;

    if (!HasAuthority() || !CognitiveBrain) {
        return Result;
    }

    float Standing = 0.0f;
    float HostileThreshold = -50.0f;
    float FriendlyThreshold = 50.0f;
    const FMythicFactionId MyFaction = CognitiveBrain->GetFaction();
    if (Interactor) {
        if (const AMythicPlayerState *PS = Interactor->GetPlayerState<AMythicPlayerState>()) {
            if (const UMythicFactionStandingComponent *Standings = PS->GetFactionStanding()) {
                HostileThreshold = Standings->GetHostileThreshold();
                FriendlyThreshold = Standings->GetFriendlyThreshold();
                if (MyFaction.IsValid()) {
                    Standing = Standings->GetStanding(MyFaction);
                }
            }
        }
    }

    Result = UMythicSocialVerbLibrary::ResolveReaction(Verb, CognitiveBrain->GetPersonality(), Standing,
                                                       HostileThreshold, FriendlyThreshold);
    return Result;
}

void AMythicNPCCharacter::ApplySocialReaction(const FMythicSocialReactionResult &Result, EMythicSocialVerb Verb, APlayerController *Interactor) {
    if (!HasAuthority()) {
        return;
    }

    LastSocialVerb = Verb;
    LastSocialReaction = Result.Reaction;
    LastSocialReactionTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    bHasSocialReaction = true;

    if (Result.StandingDelta != 0.0f && Interactor && CognitiveBrain) {
        const FMythicFactionId MyFaction = CognitiveBrain->GetFaction();
        if (MyFaction.IsValid()) {
            if (AMythicPlayerState *PS = Interactor->GetPlayerState<AMythicPlayerState>()) {
                if (UMythicFactionStandingComponent *Standings = PS->GetFactionStanding()) {
                    Standings->ServerAdjustStanding(MyFaction, Result.StandingDelta);
                }
            }
        }
    }

    APawn *InteractorPawn = Interactor ? Interactor->GetPawn() : nullptr;

    if (Result.bSetHostile && InteractorPawn) {
        if (AMythicAIController *AI = Cast<AMythicAIController>(GetController())) {
            AI->ForceEngageTarget(InteractorPawn);
        }
    }

    if (Result.bAlertGuards && InteractorPawn) {
        const float RadiusSq = GuardAlertRadius * GuardAlertRadius;
        const FVector MyLoc = GetActorLocation();
        int32 Roused = 0;
        for (TActorIterator<AMythicNPCCharacter> It(GetWorld()); It; ++It) {
            if (Roused >= GuardAlertMaxResponders) {
                break;
            }
            AMythicNPCCharacter *Responder = *It;
            if (!IsValid(Responder) || Responder == this) {
                continue;
            }
            if (GuardAlertRadius > 0.0f &&
                FVector::DistSquared(MyLoc, Responder->GetActorLocation()) > RadiusSq) {
                continue;
            }
            AMythicAIController *RespAI = Cast<AMythicAIController>(Responder->GetController());
            if (!RespAI) {
                continue;
            }
            if (RespAI->GetTeamAttitudeTowards(*InteractorPawn) != ETeamAttitude::Hostile) {
                continue;
            }
            if (Responder->CognitiveBrain) {
                Responder->CognitiveBrain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK,
                                                              Responder->CognitiveBrain->GetHomeCell());
            }
            RespAI->ForceEngageTarget(InteractorPawn);
            ++Roused;
        }
    }
}

void AMythicNPCCharacter::FireReaction(EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line, APlayerController *Interactor) {
    OnNpcReaction(Verb, Reaction, Line, Interactor);
}


void AMythicNPCCharacter::ServerSetActivity(FGameplayTag ActivityTag) {
    if (!HasAuthority()) {
        return;
    }
    if (CurrentActivityTag == ActivityTag) {
        return;
    }
    CurrentActivityTag = ActivityTag;
    if (EntityPresentationComponent) {
        if (ActivityTag.IsValid()) {
            EntityPresentationComponent->SetObservableFact(
                MythicEntityPresentationTags::ObservableSlotActivity,
                ActivityTag, FMythicPresentationHandle());
        }
        else {
            EntityPresentationComponent->ClearObservableFact(
                MythicEntityPresentationTags::ObservableSlotActivity);
        }
    }
    Multicast_PerformActivity(ActivityTag);
}

void AMythicNPCCharacter::Multicast_PerformActivity_Implementation(FGameplayTag ActivityTag) {
    OnPerformActivity(ActivityTag);
}

void AMythicNPCCharacter::OnSecondaryInteract_Implementation(AActor *Interactor) {
    if (!IsMerchant()) {
        return;
    }
    AController *C = Cast<AController>(Interactor);
    if (!C) {
        if (const APawn *P = Cast<APawn>(Interactor)) {
            C = P->GetController();
        }
    }
    APlayerController *PC = Cast<APlayerController>(C);
    if (PC && PC->IsLocalController()) {
        OnTradeOpened(PC);
    }
}

bool AMythicNPCCharacter::IsActorInTradeRange(const AActor *Actor) const {
    if (TradeRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= TradeRangeSq;
}

USceneComponent *AMythicNPCCharacter::GetWidgetAttachmentComponent_Implementation() const {
    return GetCapsuleComponent();
}

bool AMythicNPCCharacter::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    if (!CognitiveBrain) {
        return false;
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicNPCCharacter::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicNPCCharacter::OnUnfocused_Implementation(AActor *Interactor) {
}

UAbilitySystemComponent *AMythicNPCCharacter::GetAbilitySystemComponent() const {
    return AbilitySystemComponent;
}

void AMythicNPCCharacter::InitializeASC() {
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    CapturePristineAttributeBases();

    if (LifeComponent && !LifeComponent->IsInitialized()) {
        LifeComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
    }

    if (HasAuthority() && LifeComponent) {
        // Authoritative teardown uses native delegates. Blueprint lifecycle
        // events remain presentation hooks, but cannot strand a live actor if
        // their serialized invocation list is reset or reinstanced.
        LifeComponent->OnDeathNative.RemoveAll(this);
        LifeComponent->OnDownedNative.RemoveAll(this);
        LifeComponent->OnRevivedNative.RemoveAll(this);
        LifeComponent->OnDeathNative.AddUObject(
            this, &AMythicNPCCharacter::HandleNPCDeath);
        LifeComponent->OnDownedNative.AddUObject(
            this, &AMythicNPCCharacter::HandleNPCDowned);
        LifeComponent->OnRevivedNative.AddUObject(
            this, &AMythicNPCCharacter::HandleNPCRevived);
    }
}

void AMythicNPCCharacter::HandleNPCDowned(AActor *DownedActor) {
    if (!HasAuthority() || DownedActor != this || !EntityPresentationComponent) {
        return;
    }
    EntityPresentationComponent->SetObservableFact(
        MythicEntityPresentationTags::ObservableSlotLifeState,
        MythicEntityPresentationTags::ObservableLifeDowned,
        FMythicPresentationHandle());
}

void AMythicNPCCharacter::HandleNPCRevived(AActor *RevivedActor) {
    if (!HasAuthority() || RevivedActor != this || !EntityPresentationComponent) {
        return;
    }
    EntityPresentationComponent->ClearObservableFact(
        MythicEntityPresentationTags::ObservableSlotLifeState);
}

void AMythicNPCCharacter::HandleNPCDeath(AActor *DeadActor) {
    if (!HasAuthority()) {
        return;
    }

    if (CognitiveBrain) {
        CognitiveBrain->StopThinking();
    }

    if (UMythicPartySubsystem *Party = GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
        Party->RemoveCompanionFromAnyParty(this, false);
    }

    // Resolve the canonical person from its Mass fragment or, for manager-owned authored/runtime actors, the private
    // NPC data stamped by the authority allocator. Every actual person death receives a tombstone before its body is
    // recycled; otherwise a learned dossier could retain a live-looking identity whose only embodiment was erased.
    const FMassEntityHandle Entity = CognitiveBrain ? CognitiveBrain->GetSourceEntity() : FMassEntityHandle();
    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    FMythicEntityId DeadEntityId;
    FMythicFactionId DeadFaction;
    FGameplayTag DeadRole;
    FMythicCellCoord DeadCell;
    if (EntitySubsystem && Entity.IsSet() && EntitySubsystem->GetEntityManager().IsEntityValid(Entity)) {
        if (const FMythicIdentityFragment *Id =
            EntitySubsystem->GetEntityManager().GetFragmentDataPtr<FMythicIdentityFragment>(Entity)) {
            DeadEntityId = Id->EntityId;
            DeadFaction = Id->Faction;
            DeadRole = Id->RoleTag;
            DeadCell = Id->Cell;
        }
    }
    if (!DeadEntityId.IsValid() && NPCData.EntityId.IsValid()) {
        DeadEntityId = NPCData.EntityId;
        if (CognitiveBrain) {
            DeadFaction = CognitiveBrain->GetFaction();
            DeadRole = CognitiveBrain->GetRole();
            DeadCell = CognitiveBrain->GetHomeCell();
        }
    }
    if (!DeadEntityId.IsValid() && EntityPresentationComponent
        && EntityPresentationComponent->GetAuthorityEntityId().GetDomain()
               == EMythicEntityDomain::LivingWorld) {
        DeadEntityId = EntityPresentationComponent->GetAuthorityEntityId();
    }
    if (DeadEntityId.IsValid() && CognitiveBrain) {
        if (!DeadFaction.IsValid()) {
            DeadFaction = CognitiveBrain->GetFaction();
        }
        if (!DeadRole.IsValid()) {
            DeadRole = CognitiveBrain->GetRole();
        }
        DeadCell = CognitiveBrain->GetHomeCell();
    }

    if (DeadEntityId.IsValid()) {
        if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
            if (UMythicLivingWorldSubsystem *LWS =
                    GI->GetSubsystem<UMythicLivingWorldSubsystem>();
                LWS && LWS->IsSystemActive()) {
                if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                    DeadCell = Grid->WorldToCell(GetActorLocation());
                }
                if (UMythicPersistentNPCRegistry *Reg =
                        LWS->GetPersistentNPCRegistry();
                    Reg && Reg->RegisterDeath(
                               DeadEntityId, DeadFaction, DeadRole, DeadCell,
                               GetWorld()->GetTimeSeconds(), LWS)) {
                    LWS->ReportNpcDeath(DeadFaction, DeadRole);
                }
            }
        }
    }

    // A corpse is not the live combatant's nameplate with a skull appended. Retire the live presentation now; any
    // loot, revive, harvest, or inspect affordance must project from its own actionable corpse context.
    if (EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
    }

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(CorpseTimerHandle,
                                          FTimerDelegate::CreateWeakLambda(this, [this]() {
                                              // The manager reclaims its own; anything else (Mass embodiment)
                                              // owns its lifecycle and tears down as before.
                                              if (const UWorld *W = GetWorld()) {
                                                  UGameInstance *GI = W->GetGameInstance();
                                                  if (UMythicNPCManager *Mgr = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr) {
                                                      if (Mgr->ReclaimNPC(this)) {
                                                          return;
                                                      }
                                                  }
                                              }
                                              Destroy();
                                          }),
                                          CorpseLifetime, false);
    }
}

void AMythicNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (HasAuthority()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicPartySubsystem *Party = World->GetSubsystem<UMythicPartySubsystem>()) {
                Party->RemoveCompanionFromAnyParty(this, false);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AMythicNPCCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicNPCCharacter, AbilitySystemComponent);
    DOREPLIFETIME(AMythicNPCCharacter, Appearance);
}

void AMythicNPCCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);
    InitializeASC();
}

void AMythicNPCCharacter::BeginPlay() {
    Super::BeginPlay();

    const UMythicAgentDetailSettings *AgentDetail = GetDefault<UMythicAgentDetailSettings>();
    const FMythicAgentRotationConfig &Rotation = AgentDetail->NPCRotation;
    bUseControllerRotationYaw = !Rotation.bRotateInMovementComponent;
    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        CMC->bUseControllerDesiredRotation = Rotation.bRotateInMovementComponent;
        CMC->RotationRate = UMythicAgentDetailSettings::MakeRotationRate(Rotation);
        CMC->bAlwaysCheckFloor = AgentDetail->NPCFloor.bAlwaysCheckFloor;
    }

    InitializeASC();
    ConfigureEntityPresentationAnchor();

    // Manager, Mass, and designer spawners stamp their canonical identity synchronously after spawning. Defer the
    // fallback one tick so those richer paths always win; only a genuinely direct actor reaches the typed fallback.
    if (HasAuthority()) {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        SetActorTickEnabled(false);
        ParkMovementForPool();
        GetWorldTimerManager().SetTimerForNextTick(
            this, &ThisClass::TryActivateDirectEntityPresentation);
    }
}

void AMythicNPCCharacter::ConfigureEntityPresentationAnchor() {
    if (!EntityPresentationComponent) {
        return;
    }

    UCapsuleComponent *Capsule = GetCapsuleComponent();
    const float HeadClearance = Capsule
        ? Capsule->GetUnscaledCapsuleHalfHeight() + 28.0f : 120.0f;
    EntityPresentationComponent->SetPresentationAnchor(
        Capsule ? Cast<USceneComponent>(Capsule) : GetRootComponent(),
        FVector(0.0f, 0.0f, HeadClearance));
}

void AMythicNPCCharacter::BuildDirectPublicIdentity(
    FMythicPublicIdentitySnapshot &OutIdentity) const {
    OutIdentity.Reset();
    OutIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    OutIdentity.PublicIdentityDefinitionId =
        UMythicEntityIdentityDefinition::ResolvePrimaryAssetId(
            NPCData.PublicIdentityDefinition);
}

void AMythicNPCCharacter::TryActivateDirectEntityPresentation() {
    if (!HasAuthority() || !EntityPresentationComponent
        || EntityPresentationComponent->GetAuthorityEntityId().IsValid()
        || EntityPresentationComponent->GetPublicIdentitySnapshot().IsActive()) {
        return;
    }

    const bool bAuthoredWorldActor = AuthoredWorldIdentityGuid.IsValid();
    const FGuid IdentityGuid = bAuthoredWorldActor
        ? AuthoredWorldIdentityGuid : FGuid::NewGuid();
    const FMythicEntityId EntityId = FMythicEntityId::FromAuthorityGuid(
        bAuthoredWorldActor ? EMythicEntityDomain::AuthoredWorld
                            : EMythicEntityDomain::Runtime,
        IdentityGuid);
    if (!EntityId.IsValid()) {
        return;
    }
    NPCData.EntityId = EntityId;

    FMythicPublicIdentitySnapshot SafeIdentity;
    BuildDirectPublicIdentity(SafeIdentity);
    if (!EntityPresentationComponent->AuthorityPrepareEmbodiment(
            EntityId, SafeIdentity)) {
        UE_LOG(Myth, Warning,
               TEXT("Direct entity presentation could not prepare %s."),
               *GetNameSafe(this));
        return;
    }

    if (!ActivatePreparedEmbodiment()) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
        UE_LOG(Myth, Warning,
               TEXT("Direct entity presentation could not activate %s."),
               *GetNameSafe(this));
    }
}

#if WITH_EDITOR
void AMythicNPCCharacter::RefreshAuthoredWorldIdentityFromActorGuid() {
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
        || IsRunningCommandlet()) {
        return;
    }
    const UWorld *World = GetWorld();
    if (!GIsEditor || !World || World->IsGameWorld()
        || !GetActorGuid().IsValid()
        || AuthoredWorldIdentityGuid == GetActorGuid()) {
        return;
    }

    AuthoredWorldIdentityGuid = GetActorGuid();
    MarkPackageDirty();
}

void AMythicNPCCharacter::PostLoad() {
    Super::PostLoad();
    RefreshAuthoredWorldIdentityFromActorGuid();
}

void AMythicNPCCharacter::PostActorCreated() {
    Super::PostActorCreated();
    RefreshAuthoredWorldIdentityFromActorGuid();
}

void AMythicNPCCharacter::PostEditImport() {
    Super::PostEditImport();
    // The editor assigns a fresh ActorGuid to copies; mirror it so duplicate world actors can never alias identity.
    AuthoredWorldIdentityGuid.Invalidate();
    RefreshAuthoredWorldIdentityFromActorGuid();
}
#endif

void AMythicNPCCharacter::InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) {
    if (!HasAuthority()) {
        return;
    }

    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    if (!EntitySubsystem || !EntitySubsystem->GetEntityManager().IsEntityValid(InEntityHandle)) {
        return;
    }

    const FMassEntityManager &EntityManager = EntitySubsystem->GetEntityManager();

    const FMythicIdentityFragment *IdentityFrag = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(InEntityHandle);
    const FMythicPersonalityFragment *PersonalityFrag = EntityManager.GetFragmentDataPtr<FMythicPersonalityFragment>(InEntityHandle);
    const FMythicScheduleFragment *ScheduleFrag = EntityManager.GetFragmentDataPtr<FMythicScheduleFragment>(InEntityHandle);

    if (CognitiveBrain && IdentityFrag && PersonalityFrag && ScheduleFrag) {
        CognitiveBrain->InitializeBrain(
            IdentityFrag->Faction,
            ScheduleFrag->HomeCell,
            *PersonalityFrag,
            InEntityHandle,
            IdentityFrag->TrueFaction,
            IdentityFrag->RoleTag
            );
    }

    if (IdentityFrag) {
        NPCData.EntityId = IdentityFrag->EntityId;
        ApplyAppearanceFromIdentity(*IdentityFrag);

        if (EntityPresentationComponent) {
            FMythicPublicIdentitySnapshot SafeIdentity;
            BuildMassPublicIdentity(*IdentityFrag, SafeIdentity);
            EntityPresentationComponent->AuthorityPrepareEmbodiment(
                IdentityFrag->EntityId, SafeIdentity);
        }
    }

    // Mass embodiment never passes through the NPC manager, so the level stamp happens here: the territory
    // danger at the embodiment site through the same resolver, then the scaling GE re-applies with it.
    StampCombatLevel(MythicCombat::ResolveCombatLevelAt(GetWorld(), GetActorLocation()));
}

void AMythicNPCCharacter::BuildMassPublicIdentity(
    const FMythicIdentityFragment & /*Identity*/,
    FMythicPublicIdentitySnapshot &OutIdentity) const {
    OutIdentity.Reset();
    OutIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    // Mass role/faction are private simulation truth. A future cover fragment may explicitly opt safe values in;
    // absent that authoring, recognition grants are the only path to learned role and faction.
}

void AMythicNPCCharacter::StampCombatLevel(const int32 Level) {
    if (!HasAuthority()) {
        return;
    }
    NPCData.CombatLevel = FMath::Max(1, Level);
    if (bCombatInitialized) {
        ApplyCombatScaling(true);
    }
}

void AMythicNPCCharacter::OnRep_Appearance() {
    OnApplyAppearance(Appearance);
}

void AMythicNPCCharacter::ApplyAppearanceFromIdentity(const FMythicIdentityFragment &Id) {
    if (!HasAuthority()) {
        return;
    }

    FColor PrimaryColor = FColor::White;
    const uint8 FactionIndex = Id.Faction.Index;

    UMythicLivingWorldSubsystem *LWS = nullptr;
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        }
    }

    if (LWS) {
        if (const UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase()) {
            FMythicFactionData FactionData;
            if (Id.Faction.IsValid() && FactionDB->GetFaction(Id.Faction, FactionData)) {
                PrimaryColor = MythicFactionColor::GetFactionColor(FactionData, FactionIndex);
            } else if (Id.Faction.IsValid()) {
                PrimaryColor = MythicFactionColor::DeterministicColorForId(FactionIndex);
            }
        }
    }

    const FColor SecondaryColor = (FLinearColor(PrimaryColor) * 0.6f).ToFColor(true);

    TConstArrayView<FMythicOutfitSet> OutfitSets = MythicAppearanceDefaults::GetCodeDefaultOutfitSets();
    TConstArrayView<FColor> SkinPalette = MythicAppearanceDefaults::GetCodeDefaultSkinTonePalette();
    TConstArrayView<FColor> HairPalette = MythicAppearanceDefaults::GetCodeDefaultHairTonePalette();

    if (LWS) {
        if (const UMythicLivingWorldSettings *Settings = LWS->GetSettings()) {
            if (!Settings->AppearanceLibrary.IsNull()) {
                if (const UMythicAppearanceLibrary *Library = Settings->AppearanceLibrary.LoadSynchronous()) {
                    if (Library->OutfitSets.Num() > 0) {
                        OutfitSets = Library->OutfitSets;
                    }
                }
            }
            if (Settings->DefaultSkinTonePalette.Num() > 0) {
                SkinPalette = Settings->DefaultSkinTonePalette;
            }
            if (Settings->DefaultHairTonePalette.Num() > 0) {
                HairPalette = Settings->DefaultHairTonePalette;
            }
        }
    }

    const uint8 WealthTier = FMythicAppearanceResolver::WealthTierFromHash(Id.NameSeed);
    const FMythicAppearance Resolved = FMythicAppearanceResolver::Resolve(
        Id.NameSeed,
        Id.DemographicFlags,
        Id.RoleTag,
        FactionIndex,
        WealthTier,
        PrimaryColor,
        SecondaryColor,
        OutfitSets,
        SkinPalette,
        HairPalette);

    Appearance = Resolved;

    OnApplyAppearance(Appearance);
}
