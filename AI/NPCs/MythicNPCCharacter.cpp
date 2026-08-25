

#include "MythicNPCCharacter.h"

#include "MythicNPCManager.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "Settings/MythicCombatSettings.h"
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
#include "AI/NPCs/MythicAIController.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "EngineUtils.h"


const FMythicNPCData AMythicNPCCharacter::GetNPCData() const {
    return this->NPCData;
}

void AMythicNPCCharacter::OnSpawnedFromPool(const struct FMythicNPCData &InNPCData) {
    if (!this->HasAuthority()) {
        return;
    }
    this->NPCData = InNPCData;

    // Waking from the pool: visible and solid again before anything else runs.
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(PrimaryActorTick.bCanEverTick);
    RestoreMovementFromPool();

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

    SeedAttributesFromData();

    // Possession inside SpawnActor already ran CombatInit with a default-constructed NPCData, so the
    // scaling GE holds level-1 multipliers. Now that the stamped data is in, apply the real ones.
    ApplyCombatScaling();

    GrantAttackAbility();

    if (AbilitySystemComponent && !NPCData.Traits.IsEmpty()) {
        AbilitySystemComponent->AddLooseGameplayTags(NPCData.Traits);
    }

    InitializeBrainFromNPCData();

    if (LifeAttributes) {
        LifeAttributes->ResetForRespawn();
    }

    if (LifeComponent) {
        LifeComponent->RestoreAfterDeath();
    }
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
    CognitiveBrain->StartThinking();
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

void AMythicNPCCharacter::CombatInit() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
    }

    GrantAttackAbility();

    if (bCombatInitialized) {
        return;
    }
    for (const TSubclassOf<UGameplayEffect> &Effect : DefaultGameplayEffects) {
        if (!Effect) {
            continue;
        }
        FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
        Ctx.AddSourceObject(this);
        const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(Effect, 1.0f, Ctx);
        if (Spec.IsValid()) {
            const FActiveGameplayEffectHandle Applied = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            if (Applied.IsValid()) {
                DefaultEffectHandles.Add(Applied);
            }
        }
    }

    if (DefaultGameplayEffects.Num() > 0) {
        for (const FRolledAttributeSpec &Roll : NPCData.Proficiencies) {
            if (!Roll.Attribute.IsValid() || !AbilitySystemComponent->HasAttributeSetForAttribute(Roll.Attribute)) {
                continue;
            }
            const UClass *SetClass = Roll.Attribute.GetAttributeSetClass();
            if (!SetClass) {
                continue;
            }
            const UAttributeSet *CDO = SetClass->GetDefaultObject<UAttributeSet>();
            const float CdoDefault = Roll.Attribute.GetNumericValue(CDO);
            const float CurrentBase = AbilitySystemComponent->GetNumericAttributeBase(Roll.Attribute);
            if (!FMath::IsNearlyEqual(CurrentBase, CdoDefault)) {
                UE_LOG(Myth, Warning,
                       TEXT("AMythicNPCCharacter::CombatInit: %s attribute %s is set by BOTH a DefaultGameplayEffect ")
                       TEXT("(base=%.2f) and an NPCData.Proficiency; the proficiency's CDO reset in SeedAttributesFromData ")
                       TEXT("will discard the DefaultGE baseline. Keep the two layers on disjoint attributes."),
                       *GetNameSafe(this), *Roll.Attribute.GetName(), CurrentBase);
            }
        }
    }

    ApplyCombatScaling();

    PublishIdentityTags();

    ApplyMonsterAffixes();

    bCombatInitialized = true;
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

    if (CreatureKind.IsValid()) {
        AbilitySystemComponent->AddLooseGameplayTag(CreatureKind);
    }

    if (EnemyTier.IsValid()) {
        AbilitySystemComponent->AddLooseGameplayTag(EnemyTier);
    }

    if (CodexBestiaryKey.IsValid()) {
        AbilitySystemComponent->AddLooseGameplayTag(CodexBestiaryKey);
    }
}

void AMythicNPCCharacter::ApplyCombatScaling() {
    if (!HasAuthority() || !AbilitySystemComponent) {
        return;
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

    // The level half: the GameState's min/max curves sampled at this NPC's combat level, rolled once per spawn so
    // two same-level wolves are not clones. Unauthored curves read 1.0 and the level term vanishes.
    //
    // Combatants only. A living world spawns merchants, farmers and wanderers from these same pools, and their
    // disposition is flee-or-talk, not fight - an authored attack ability is what marks an entity as combat-trained, so a
    // civilian keeps its authored stats no matter how dangerous its home territory is. CombatLevel
    // still stamps on everyone: it is world context (loot, trade stock, XP), not a threat statement.
    float LevelHealthMult = 1.0f;
    float LevelDamageMult = 1.0f;
    if (const UWorld *World = AttackAbility ? GetWorld() : nullptr) {
        if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
            const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
            const float Level = static_cast<float>(FMath::Max(1, NPCData.CombatLevel));
            const float HealthLow = MythicCombat::SampleOpenEnded(GS->HealthMinCurveRowHandle, Level, Settings->CombatantHealthTailGrowth);
            const float HealthHigh = MythicCombat::SampleOpenEnded(GS->HealthMaxCurveRowHandle, Level, Settings->CombatantHealthTailGrowth);
            const float DamageLow = MythicCombat::SampleOpenEnded(GS->DamageMinCurveRowHandle, Level, Settings->CombatantDamageTailGrowth);
            const float DamageHigh = MythicCombat::SampleOpenEnded(GS->DamageMaxCurveRowHandle, Level, Settings->CombatantDamageTailGrowth);
            LevelHealthMult = FMath::FRandRange(FMath::Min(HealthLow, HealthHigh), FMath::Max(HealthLow, HealthHigh));
            LevelDamageMult = FMath::FRandRange(FMath::Min(DamageLow, DamageHigh), FMath::Max(DamageLow, DamageHigh));
        }
    }

    const float HealthMult = static_cast<float>(PartyWorldMult.X) * Tier.HealthMult * LevelHealthMult;
    const float DamageMult = static_cast<float>(PartyWorldMult.Y) * Tier.DamageMult * LevelDamageMult;

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

    if (LifeComponent) {
        if (!bBaseXPRewardCaptured) {
            BaseXPReward = LifeComponent->XPReward;
            bBaseXPRewardCaptured = true;
        }
        LifeComponent->XPReward = BaseXPReward * Tier.XpMult;
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

        if (UClass *SetClass = Roll.Attribute.GetAttributeSetClass()) {
            const UAttributeSet *CDO = SetClass->GetDefaultObject<UAttributeSet>();
            AbilitySystemComponent->SetNumericAttributeBase(Roll.Attribute, Roll.Attribute.GetNumericValue(CDO));
        }

        AbilitySystemComponent->ApplyModToAttribute(Roll.Attribute, Roll.Definition.Modifier, Roll.Value);
    }
}

void AMythicNPCCharacter::OnReturnedToPool() {
    if (!HasAuthority()) {
        return;
    }

    if (AbilitySystemComponent) {
        AbilitySystemComponent->CancelAllAbilities();
    }

    if (AbilitySystemComponent) {
        FGameplayTagContainer AllTags;
        AbilitySystemComponent->GetOwnedGameplayTags(AllTags);
        AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(AllTags);

        AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GAS_STATE_COMBATSCALING));

        // Tag-less baseline GEs are invisible to the owned-tags sweep above; without the explicit removal
        // the next CombatInit stacks a second baseline on top.
        for (const FActiveGameplayEffectHandle &Handle : DefaultEffectHandles) {
            if (Handle.IsValid()) {
                AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
            }
        }
        DefaultEffectHandles.Reset();

        FMonsterAffixGranter::RemoveMonsterAffixes(AbilitySystemComponent, MonsterAffixHandles);
    }

    if (AbilitySystemComponent) {
        FGameplayTagContainer TempTags;
        AbilitySystemComponent->GetOwnedGameplayTags(TempTags);
        for (const FGameplayTag &Tag : TempTags) {
            AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
        }
    }

    if (LifeComponent) {
        LifeComponent->UninitializeFromAbilitySystem();
    }

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

    bCombatInitialized = false;
    if (AbilitySystemComponent && AttackAbilityHandle.IsValid()) {
        // CancelAllAbilities stops the activation; only ClearAbility releases the spec. Without this every
        // pooled reuse re-granted on top of the old grant and the spec list grew for the actor's lifetime.
        AbilitySystemComponent->ClearAbility(AttackAbilityHandle);
    }
    AttackAbilityHandle = FGameplayAbilitySpecHandle();
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

    RestoreMovementFromPool();

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

    if (LifeAttributes) {
        LifeAttributes->ResetForRespawn();
    }

    if (LifeComponent) {
        LifeComponent->RestoreAfterDeath();
    }

    SetActorTickEnabled(PrimaryActorTick.bCanEverTick);

    if (CognitiveBrain) {
        CognitiveBrain->StartThinking();
    }
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

    EnemyTier = AI_TIER_NORMAL;

    CreatureKind = AI_KIND_HUMANOID;

    CognitiveBrain = CreateDefaultSubobject<UMythicCognitiveBrainComponent>("CognitiveBrain");

    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>("LifeComponent");

    AIControllerClass = AMythicNPCAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

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

    CombatInit();

    if (LifeComponent && !LifeComponent->IsInitialized()) {
        LifeComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
    }

    if (HasAuthority() && LifeComponent && !bBoundDeath) {
        LifeComponent->OnDeath.AddDynamic(this, &AMythicNPCCharacter::HandleNPCDeath);
        bBoundDeath = true;
    }
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

    // Only Mass-embodied NPCs report into the persistent registry; the corpse timer below runs for every
    // authority death, or manager-owned NPCs would stand as ticking corpses forever.
    const FMassEntityHandle Entity = CognitiveBrain ? CognitiveBrain->GetSourceEntity() : FMassEntityHandle();
    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    if (EntitySubsystem && Entity.IsSet() && EntitySubsystem->GetEntityManager().IsEntityValid(Entity)) {
        if (const FMythicIdentityFragment *Id =
            EntitySubsystem->GetEntityManager().GetFragmentDataPtr<FMythicIdentityFragment>(Entity)) {
            if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
                if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    if (LWS->IsSystemActive()) {
                        if (UMythicPersistentNPCRegistry *Reg = LWS->GetPersistentNPCRegistry()) {
                            Reg->RegisterDeath(Id->NameHash, Id->Faction, Id->RoleTag, Id->Cell,
                                               GetWorld()->GetTimeSeconds(), LWS);
                        }
                        LWS->ReportNpcDeath(Id->Faction, Id->RoleTag);
                    }
                }
            }
        }
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
    DOREPLIFETIME(AMythicNPCCharacter, EngagedTarget);
}

void AMythicNPCCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);
    InitializeASC();
}

void AMythicNPCCharacter::BeginPlay() {
    Super::BeginPlay();

    InitializeASC();
}

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
        ApplyAppearanceFromIdentity(*IdentityFrag);
    }

    // Mass embodiment never passes through the NPC manager, so the level stamp happens here: the territory
    // danger at the embodiment site through the same resolver, then the scaling GE re-applies with it.
    StampCombatLevel(MythicCombat::ResolveCombatLevelAt(GetWorld(), GetActorLocation()));
}

void AMythicNPCCharacter::StampCombatLevel(const int32 Level) {
    if (!HasAuthority()) {
        return;
    }
    NPCData.CombatLevel = FMath::Max(1, Level);
    ApplyCombatScaling();
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

    const uint8 WealthTier = FMythicAppearanceResolver::WealthTierFromHash(Id.NameHash);
    const FMythicAppearance Resolved = FMythicAppearanceResolver::Resolve(
        Id.NameHash,
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
