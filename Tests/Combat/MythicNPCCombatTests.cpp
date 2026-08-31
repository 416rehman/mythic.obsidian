#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AI/MythicTags_AI.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/World.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "GameplayEffect.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationTags.h"

namespace {
const TCHAR *TierPawns[] = {
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Superior.Pawn_Humanoid_Superior_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Elite.Pawn_Humanoid_Elite_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Champion.Pawn_Humanoid_Champion_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Boss.Pawn_Humanoid_Boss_C"),
};

// Named rather than indexed into TierPawns: an index silently points at a different pawn the moment
// someone adds one, and the test would still pass while asserting nothing about a boss.
const TCHAR *BossPawn = TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Boss.Pawn_Humanoid_Boss_C");

const AMythicNPCCharacter *LoadPawn(const TCHAR *Path) {
    const UClass *Loaded = LoadClass<AActor>(nullptr, Path);
    return Loaded ? Cast<AMythicNPCCharacter>(Loaded->GetDefaultObject()) : nullptr;
}

class FScopedNPCCombatWorld final {
public:
    FScopedNPCCombatWorld() {
        Values = UWorld::InitializationValues()
                     .CreatePhysicsScene(false)
                     .ShouldSimulatePhysics(false)
                     .EnableTraceCollision(false)
                     .CreateNavigation(false)
                     .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::Game, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("NPCCombatInitializationTest")),
            nullptr, true, ERHIFeatureLevel::Num, &Values, true);
        if (World) {
            World->InitWorld(Values);
        }
    }

    ~FScopedNPCCombatWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues Values;
    UWorld *World = nullptr;
};

/** Highest Override magnitude an effect applies to one attribute, or -1 when it never touches it. */
float FindOverride(const UGameplayEffect *Effect, const FGameplayAttribute &Attribute) {
    if (!Effect) {
        return -1.0f;
    }
    for (const FGameplayModifierInfo &Mod : Effect->Modifiers) {
        if (Mod.Attribute == Attribute) {
            float Magnitude = 0.0f;
            Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Magnitude);
            return Magnitude;
        }
    }
    return -1.0f;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNPCsCanFightTest,
    "Mythic.Combat.NPCsCanFight",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNPCsCanFightTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: no NPC in the project could deal damage. Every tier pawn had a null
    // AttackAbility, and the only default effect any of them carried set MaxHealth and Health and nothing
    // else - so DamagePerHit, Power and Armor all sat at the C++ default of 0. Any tier multiplier still
    // leaves a zero baseline at zero. Every defensive stat in the game was therefore
    // unmeasurable in play, and it looked like broken AI rather than empty data.
    for (const TCHAR *Path : TierPawns) {
        const AMythicNPCCharacter *Pawn = LoadPawn(Path);
        const FString Name = FPaths::GetBaseFilename(FString(Path));
        if (!TestNotNull(*FString::Printf(TEXT("%s loads"), *Name), Pawn)) {
            continue;
        }

        TestNotNull(*FString::Printf(TEXT("%s carries an attack ability"), *Name),
                    Pawn->GetAttackAbility().Get());
        TestTrue(*FString::Printf(TEXT("%s carries a stat baseline"), *Name),
                 Pawn->GetDefaultGameplayEffects().Num() > 0);
        TestTrue(*FString::Printf(TEXT("%s declares a tier"), *Name),
                 Pawn->GetEnemyTier().IsValid());

        // A baseline that never sets DamagePerHit leaves it at zero, which is the whole bug: the pawn
        // has an ability, swings it, and applies nothing.
        bool bGrantsDamage = false;
        bool bGrantsArmor = false;
        for (const TSubclassOf<UGameplayEffect> &EffectClass : Pawn->GetDefaultGameplayEffects()) {
            const UGameplayEffect *Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
            if (FindOverride(Effect, UMythicAttributeSet_Offense::GetDamagePerHitAttribute()) > 0.0f) {
                bGrantsDamage = true;
            }
            if (FindOverride(Effect, UMythicAttributeSet_Defense::GetArmorAttribute()) > 0.0f) {
                bGrantsArmor = true;
            }
        }
        TestTrue(*FString::Printf(TEXT("%s is given non-zero DamagePerHit"), *Name), bGrantsDamage);
        // Armor on the attacker matters too: it is what makes the mitigation curve observable from a fight.
        TestTrue(*FString::Printf(TEXT("%s is given non-zero Armor"), *Name), bGrantsArmor);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNPCTierDamageTest,
    "Mythic.Combat.NPCTierDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNPCTierDamageTest::RunTest(const FString &Parameters) {
    // A shared baseline plus a per-tier multiplier is the only reason a Boss hits harder than a Superior.
    // Assert the product, not the multiplier alone: a baseline of zero multiplies to zero at every tier,
    // which is exactly how this shipped.
    const AMythicNPCCharacter *Boss = LoadPawn(BossPawn);
    if (!TestNotNull(TEXT("the boss pawn loads"), Boss)) {
        return false;
    }

    float Baseline = 0.0f;
    for (const TSubclassOf<UGameplayEffect> &EffectClass : Boss->GetDefaultGameplayEffects()) {
        const UGameplayEffect *Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
        Baseline = FMath::Max(Baseline, FindOverride(Effect, UMythicAttributeSet_Offense::GetDamagePerHitAttribute()));
    }
    TestTrue(TEXT("the boss has a non-zero damage baseline to scale"), Baseline > 0.0f);

    const float SuperiorHit = Baseline * FMythicEnemyScaling::GetTierScaling(AI_TIER_SUPERIOR).DamageMult;
    const float BossHit = Baseline * FMythicEnemyScaling::GetTierScaling(AI_TIER_BOSS).DamageMult;

    TestTrue(TEXT("a superior actually hits for something"), SuperiorHit > 0.0f);
    TestTrue(TEXT("a boss hits harder than a superior"), BossHit > SuperiorHit);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNPCCommittedVitalityTest,
    "Mythic.Combat.NPCCommittedVitality",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNPCCommittedVitalityTest::RunTest(const FString &Parameters) {
    static const TCHAR *DummyPawnPath =
        TEXT("/Game/Mythic/AI/NPCs/Dummy/Pawn_Dummy.Pawn_Dummy_C");

    FScopedNPCCombatWorld Fixture;
    UWorld *World = Fixture.Get();
    UClass *PawnClass = LoadClass<AMythicNPCCharacter>(nullptr, DummyPawnPath);
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMythicNPCCharacter *NPC = World && PawnClass
        ? World->SpawnActor<AMythicNPCCharacter>(
              PawnClass, FTransform::Identity, SpawnParameters)
        : nullptr;
    UAbilitySystemComponent *ASC = NPC ? NPC->GetAbilitySystemComponent() : nullptr;
    // InitWorld intentionally omits the full map-start lifecycle to keep this fixture small. Production worlds call
    // InitializeComponent before BeginPlay; reproduce that one lifecycle step so GAS discovers the pawn's four
    // default-subobject AttributeSets instead of testing an impossible half-constructed actor.
    if (ASC && !ASC->HasBeenInitialized()) {
        ASC->InitializeComponent();
    }
    if (NPC && !NPC->HasActorBegunPlay()) {
        NPC->DispatchBeginPlay();
    }
    UMythicEntityPresentationComponent *Presentation = NPC
        ? NPC->GetEntityPresentationComponent_Implementation() : nullptr;
    if (!World || !PawnClass || !NPC || !ASC || !Presentation) {
        AddError(TEXT("the authored Pawn_Dummy vitality fixture could not initialize"));
        return false;
    }

    FMythicPublicIdentitySnapshot Identity;
    Identity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    const FMythicEntityId EntityId =
        FMythicEntityId::FromAuthorityGuid(
            EMythicEntityDomain::Runtime,
            FGuid(0x65065001u, 0x65065002u, 0x65065003u, 0x65065004u));
    TestTrue(TEXT("the authority prepares an exact logical embodiment"),
             Presentation->AuthorityPrepareEmbodiment(EntityId, Identity));
    TestTrue(TEXT("the complete combat transaction activates the NPC"),
             NPC->ActivatePreparedEmbodiment());

    const float MaximumHealth = ASC->GetNumericAttribute(
        UMythicAttributeSet_Life::GetMaxHealthAttribute());
    const float Health = ASC->GetNumericAttribute(
        UMythicAttributeSet_Life::GetHealthAttribute());
    const float Armor = ASC->GetNumericAttribute(
        UMythicAttributeSet_Defense::GetArmorAttribute());

    TestTrue(TEXT("the authored 500 baseline and 10 Strength resolve to 650 MaxHealth"),
             FMath::IsNearlyEqual(MaximumHealth, 650.0f, 0.01f));
    TestTrue(TEXT("a new embodiment starts at its resolved maximum"),
             FMath::IsNearlyEqual(Health, MaximumHealth, 0.01f));
    TestTrue(TEXT("the same derivation layer resolves 10 Armor to 11.5"),
             FMath::IsNearlyEqual(Armor, 11.5f, 0.01f));
    TestFalse(TEXT("a committed NPC is visible instead of being parked by a numeric sentinel"),
              NPC->IsHidden());
    TestTrue(TEXT("a committed NPC restores collision"),
             NPC->GetActorEnableCollision());

    TInlineComponentArray<UMythicLifeComponent *> LifeComponents;
    NPC->GetComponents(LifeComponents);
    TestEqual(TEXT("an NPC has exactly one authoritative life component"),
               LifeComponents.Num(), 1);
    TestTrue(TEXT("the spawned NPC owns the authoritative death transaction"),
             NPC->HasAuthority());
    TestTrue(TEXT("the canonical life component is bound to NPC teardown"),
             LifeComponents.Num() == 1
                 && LifeComponents[0]->OnDeathNative.IsBound());

    UGameplayEffect *LethalDamage = NewObject<UGameplayEffect>(
        GetTransientPackage(), TEXT("Test_NPCLethalDamage"));
    LethalDamage->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo DamageModifier;
    DamageModifier.Attribute =
        UMythicAttributeSet_Life::GetDamageAttribute();
    DamageModifier.ModifierOp = EGameplayModOp::Additive;
    DamageModifier.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(FScalableFloat(MaximumHealth));
    LethalDamage->Modifiers.Add(DamageModifier);

    ASC->ApplyGameplayEffectToSelf(
        LethalDamage, 1.0f, ASC->MakeEffectContext());
    TestTrue(TEXT("lethal damage publishes the dead state"),
             ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestTrue(TEXT("lethal damage consumes all health"),
             FMath::IsNearlyZero(ASC->GetNumericAttribute(
                 UMythicAttributeSet_Life::GetHealthAttribute())));
    TestFalse(TEXT("death immediately retires the living nameplate identity"),
               Presentation->GetPresentationInstance().IsValid());
    return true;
}
