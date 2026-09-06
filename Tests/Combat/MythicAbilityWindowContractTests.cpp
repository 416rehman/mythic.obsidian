#include "Misc/AutomationTest.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MythicGA_AbilityWindow.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Executions/MythicDamageCalculation.h"
#include "Serialization/BitWriter.h"
#include "Serialization/BitReader.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Rewards/LootScaling.h"
#include "Settings/MythicCombatSettings.h"
#include "Tests/Combat/MythicTestEffects.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Combat/MythicAbilityWindowTestProbe.h"

namespace {
struct FAbilityCombatFixture {
    UGameInstance* Game=NewObject<UGameInstance>(GEngine);
    UWorld* World=nullptr;
    FAbilityCombatFixture() { Game->InitializeStandalone(); World=Game->GetWorld(); }
    ~FAbilityCombatFixture() { DestroyOwnedAbilityTestWorld(Game); }
    UMythicAbilitySystemComponent* Combatant(bool Defense=true) {
        auto* Actor=World->SpawnActor<AActor>();
        auto* ASC=NewObject<UMythicAbilitySystemComponent>(Actor);
        Actor->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Actor,Actor);
        ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Actor));
        ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Actor));
        if (Defense) ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Actor));
        return ASC;
    }
};
UGameplayEffect* AttributeEffect(FGameplayAttribute Attribute,float Amount,EGameplayEffectDurationType Duration=EGameplayEffectDurationType::Instant,EGameplayModOp::Type Op=EGameplayModOp::Additive) {
    auto* GE=NewObject<UGameplayEffect>(); GE->DurationPolicy=Duration;
    auto& Modifier=GE->Modifiers.AddDefaulted_GetRef(); Modifier.Attribute=Attribute;
    Modifier.ModifierOp=Op; Modifier.ModifierMagnitude=FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    return GE;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityOwnedShieldSeparationTest,"Mythic.Combat.Abilities.OwnedShieldSeparation",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityOwnedShieldSeparationTest::RunTest(const FString&) {
    FAbilityCombatFixture F;
    for (const float Damage:{0.0f,35.0f}) {
        auto* ASC=F.Combatant(); auto* Source=ASC->GetAvatarActor();
        const FGuid Activation=FGuid::NewGuid();
        const FGuid A=ASC->GrantOwnedShield(Source,Activation,TEXT("Bell"),40,6);
        const auto BeforeB=ASC->GetActiveEffects(FGameplayEffectQuery());
        const FGuid B=ASC->GrantOwnedShield(Source,Activation,TEXT("Rune"),30,10);
        TestTrue(TEXT("both source contributions are granted"),A.IsValid() && B.IsValid());
        FActiveGameplayEffectHandle MaximumB;
        for (auto Handle:ASC->GetActiveEffects(FGameplayEffectQuery())) if (!BeforeB.Contains(Handle)) MaximumB=Handle;
        if (!TestTrue(TEXT("second contribution has its own maximum effect"),MaximumB.IsValid())) return false;
        if (Damage>0) {
            // Drive the same meta boundary as canonical damage; external resource writes alone must not charge Bell.
            auto Context=ASC->MakeEffectContext(); ASC->BeginOwnedShieldDamage();
            ASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(),70-Damage);
            ASC->ConsumeOwnedShieldDamage(Damage,Context);
        }
        ASC->RemoveActiveGameplayEffect(MaximumB);
        TestEqual(TEXT("external removal leaves the other source's exact residual"),ASC->GetOwnedShieldRemaining(A),40-Damage);
        TestEqual(TEXT("external removal does not subtract its cap loss twice"),ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()),40-Damage);
        TestEqual(TEXT("removed source has no residual"),ASC->GetOwnedShieldRemaining(B),0.0f);
        ASC->GrantOwnedShield(Source,Activation,TEXT("Bell"),0,6);
        TestEqual(TEXT("a zero replacement removes this pulse's previous shield"),ASC->GetOwnedShieldRemaining(A),0.0f);
        TestEqual(TEXT("zero replacement leaves no owned shield"),ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()),0.0f);
    }
    auto* ASC=F.Combatant();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetMaxShieldAttribute(),1000);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(),10);
    ASC->ApplyGameplayEffectToSelf(AttributeEffect(UMythicAttributeSet_Defense::GetShieldAttribute(),2,EGameplayEffectDurationType::Infinite,EGameplayModOp::Multiplicitive),1,ASC->MakeEffectContext());
    const auto Shield=ASC->GrantOwnedShield(ASC->GetAvatarActor(),FGuid::NewGuid(),TEXT("Dawn"),20,4);
    TestEqual(TEXT("grant amounts use evaluated shield units"),ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()),40.0f);
    ASC->RemoveOwnedShield(Shield);
    TestEqual(TEXT("removal preserves unrelated shield under a multiplier"),ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()),20.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityShieldHitReceiptTest,"Mythic.Combat.Abilities.ShieldHitReceipt",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityShieldHitReceiptTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* Source=F.Combatant(); auto* Target=F.Combatant();
    Source->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),10);
    const auto A=Target->GrantOwnedShield(Source->GetAvatarActor(),FGuid::NewGuid(),TEXT("Bell"),1000,6);
    const auto B=Target->GrantOwnedShield(Target->GetAvatarActor(),FGuid::NewGuid(),TEXT("Rune"),1000,10);
    int32 Receipts=0; float Absorbed=0;
    Target->OnOwnedShieldAbsorbed.AddLambda([&](FGuid Id,float Amount,const FGameplayEffectContextHandle&) {
        ++Receipts; Absorbed+=Amount;
        TestEqual(TEXT("the earliest-expiring owner receives the absorption"),Id,A);
        TestEqual(TEXT("health is settled before a shield reaction"),Target->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()),100.0f);
        Target->RemoveOwnedShield(A); // Reentrant expiry/conversion may mutate the ledger during publication.
    });
    auto* GE=NewObject<UGameplayEffect>(); GE->DurationPolicy=EGameplayEffectDurationType::Instant;
    GE->Executions.AddDefaulted_GetRef().CalculationClass=UMythicDamageApplication::StaticClass();
    auto Context=Source->MakeEffectContext();
    FMythicGameplayEffectContext::ExtractEffectContext(Context)->DamageOrigin=EMythicDamageOrigin::DirectWeapon;
    Source->ApplyGameplayEffectToTarget(GE,Target,1,Context);
    TestTrue(TEXT("a canonical hit produces an actual absorption receipt"),Absorbed>0);
    TestEqual(TEXT("one hit publishes one source receipt"),Receipts,1);
    TestEqual(TEXT("reentrant removal cannot spend another shield"),Target->GetOwnedShieldRemaining(B),1000.0f);
    TestEqual(TEXT("the other shield remains in the attribute"),Target->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()),1000.0f);
    Target->PublishOwnedShieldDamage(Context);
    TestEqual(TEXT("the same context cannot publish its receipt twice"),Receipts,1);
    Target->OnOwnedShieldAbsorbed.Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityHealReceiptTest,"Mythic.Combat.Abilities.HealReceiptReentrancy",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityHealReceiptTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* ASC=F.Combatant();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),90);
    auto* Heal=AttributeEffect(UMythicAttributeSet_Life::GetHealingAttribute(),50);
    int32 Calls=0;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).AddLambda([&](const FGameplayEventData*) {
        if (++Calls==1) {
            ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),10);
            ASC->ApplyGameplayEffectToSelf(Heal,1,ASC->MakeEffectContext());
        }
    });
    auto Context=ASC->MakeEffectContext(); ASC->ApplyGameplayEffectToSelf(Heal,1,Context);
    const auto* Receipt=FMythicGameplayEffectContext::ExtractEffectContext(Context);
    TestEqual(TEXT("the original heal records its own actual gain"),Receipt->ResolvedHealing.Gained,10.0f);
    TestEqual(TEXT("overheal belongs only to that resolved application"),Receipt->ResolvedHealing.Unused,40.0f);
    TestEqual(TEXT("nested healing really ran after the first heal"),ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()),60.0f);
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityLootFractionTest,"Mythic.Itemization.Abilities.CrownFractionalDrops",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityLootFractionTest::RunTest(const FString&) {
    FLootTierBonus Bonus; Bonus.AbilityDropScale=1.25f;
    TestEqual(TEXT("a one-item roll can earn its fractional ability bonus"),FMythicLootScaling::ResolveDropCount(1,Bonus,1,.24f),2);
    TestEqual(TEXT("the remaining rolls retain the ordinary item"),FMythicLootScaling::ResolveDropCount(1,Bonus,1,.26f),1);
    Bonus.DropCountScale=0;
    TestEqual(TEXT("Crown cannot defeat a rune's no-drop rule"),FMythicLootScaling::ResolveDropCount(1,Bonus,1,0),0);
    Bonus.DropCountScale=1; Bonus.AbilityDropScale=100;
    TestEqual(TEXT("ability contribution is capped at25 percent"),FMythicLootScaling::ResolveDropCount(4,Bonus,1,1),5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityDirectControlTest,"Mythic.Combat.Abilities.DirectControlAdmission",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityDirectControlTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* ASC=F.Combatant(); auto* Source=F.Combatant();
    const auto* Stun=LoadObject<UMythicStatusEffectDefinition>(nullptr,TEXT("/Game/Mythic/GAS/Statuses/DA_Status_Stun.DA_Status_Stun"));
    if (!TestNotNull(TEXT("canonical stun is authored"),Stun)) return false;
    const auto Config=FMythicCrowdControlRules::ConfigForTier(GetDefault<UMythicCombatSettings>()->CcEscalationByTier,-1);
    int32 ApplyEvents=0;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag(TEXT("Event.ApplyStatus"))).AddLambda([&](const FGameplayEventData*){++ApplyEvents;});
    TestFalse(TEXT("actorless status without a world source is rejected"),UMythicStatusRegistry::ApplyStatusEffect(ASC,Stun,nullptr,nullptr,1));
    TestEqual(TEXT("failed status does not advertise an application"),ApplyEvents,0);
    for (int32 i=1;i<Config.ImmunityTriggerCount;++i) {
        TestTrue(TEXT("valid direct control participates in the common escalation window"),UMythicStatusRegistry::ApplyStatusEffect(ASC,Stun,Source->GetAvatarActor(),nullptr,1));
        const auto Times=ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Stun->GrantedStateTag.GetSingleTagContainer()));
        TestTrue(TEXT("authored one-second duration reaches the effect"),Times.ContainsByPredicate([](float Seconds){return Seconds>0 && Seconds<=1.01f;}));
        ASC->RemoveActiveEffectsWithGrantedTags(Stun->GrantedStateTag.GetSingleTagContainer());
    }
    TestFalse(TEXT("the immunity-triggering application does not stun"),UMythicStatusRegistry::ApplyStatusEffect(ASC,Stun,Source->GetAvatarActor(),nullptr,1));
    TestTrue(TEXT("direct ability control grants canonical immunity"),ASC->HasMatchingGameplayTag(GAS_IMMUNE_HARDCC));
    TestFalse(TEXT("immunity blocks the next direct application"),UMythicStatusRegistry::ApplyStatusEffect(ASC,Stun,Source->GetAvatarActor(),nullptr,1));
    TestEqual(TEXT("only successful applications publish ApplyStatus"),ApplyEvents,Config.ImmunityTriggerCount-1);
    auto* NoDefense=F.Combatant(false);
    TestFalse(TEXT("missing control policy fails closed"),UMythicStatusRegistry::ApplyStatusEffect(NoDefense,Stun,Source->GetAvatarActor(),nullptr,1));
    ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag(TEXT("Event.ApplyStatus"))).Clear();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityCueOwnershipTest,"Mythic.Combat.Abilities.SharedRecipientCueOwnership",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityCueOwnershipTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* Target=F.Combatant(); auto* A=F.Combatant(); auto* B=F.Combatant();
    const FGameplayTag Tag=FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Ability.BellOfLastStand.WindowTarget"));
    FGameplayCueParameters First; First.Instigator=A->GetAvatarActor(); First.EffectContext=A->MakeEffectContext();
    FGameplayCueParameters Second; Second.Instigator=B->GetAvatarActor(); Second.EffectContext=B->MakeEffectContext();
    FMythicGameplayEffectContext::ExtractEffectContext(First.EffectContext)->SourceActivationId=FGuid::NewGuid();
    FMythicGameplayEffectContext::ExtractEffectContext(Second.EffectContext)->SourceActivationId=FGuid::NewGuid();
    const int32 FirstId=Target->AddAbilityGameplayCue(Tag,First),SecondId=Target->AddAbilityGameplayCue(Tag,Second);
    TestTrue(TEXT("two allied casters retain distinct cue entries"),FirstId!=INDEX_NONE && SecondId!=INDEX_NONE && FirstId!=SecondId);
    TestEqual(TEXT("both cues are represented in GAS"),Target->GetTagCount(Tag),2);
    TestEqual(TEXT("repeating the same activation does not add a cue"),Target->AddAbilityGameplayCue(Tag,First),FirstId);
    auto* Window=NewObject<UMythicAbilityWindowTestProbe>(Target->GetOwner());
    Window->bCleaned=false; Window->Cues.Add({Target,Tag,FirstId});
    Window->InvalidatedActors.Add(Target->GetAvatarActor());
    Window->CleanupWindow();
    TestEqual(TEXT("the second caster's cue remains"),Target->GetTagCount(Tag),1);
    TestFalse(TEXT("duplicate cleanup cannot remove another caster's cue"),Target->RemoveAbilityGameplayCue(FirstId));
    Target->InitAbilityActorInfo(Target->GetOwner(),F.World->SpawnActor<AActor>());
    TestFalse(TEXT("avatar replacement cannot resurrect a removed source cue"),Target->RemoveAbilityGameplayCue(FirstId));
    TestTrue(TEXT("second caster can independently end its cue"),Target->RemoveAbilityGameplayCue(SecondId));
    TestEqual(TEXT("all owned cue state is released"),Target->GetTagCount(Tag),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityTalentDurationTest,"Mythic.Combat.Abilities.TalentStatusDuration",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityTalentDurationTest::RunTest(const FString&) {
    FAbilityCombatFixture F;
    auto* Player=F.World->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* ASC=Player->GetMythicAbilitySystemComponent();
    const auto* Slow=LoadObject<UMythicStatusEffectDefinition>(nullptr,TEXT("/Game/Mythic/GAS/Statuses/DA_Status_Slow.DA_Status_Slow"));
    if (!TestNotNull(TEXT("canonical slow is authored"),Slow)) return false;
    const auto Handle=ASC->GiveAbility(FGameplayAbilitySpec(UMythicTestTriggeredAbility::StaticClass(),1,INDEX_NONE,Player));
    auto* Spec=ASC->FindAbilitySpecFromHandle(Handle);
    auto* Talent=Spec ? Cast<UMythicGA_Triggered>(Spec->GetPrimaryInstance()):nullptr;
    if (!TestNotNull(TEXT("talent has an instanced event listener"),Talent)) return false;
    auto& Clause=Talent->Triggers[0]; Clause.Target=EMythicTriggerTarget::Self;
    Clause.StatusToApply=Slow->StatusType; Clause.Chance=1; Clause.Duration=1.25f;
    ASC->TryActivateAbility(Handle);
    FGameplayEventData Event; Event.EventTag=GAS_EVENT_KILL; Event.Instigator=Player;
    Event.ContextHandle=ASC->MakeEffectContext();
    auto* Parent=FMythicGameplayEffectContext::ExtractEffectContext(Event.ContextHandle);
    Parent->SourceActivationId=FGuid::NewGuid(); Parent->ReactionGeneration=2;
    Parent->ActionSnapshot=MakeShared<FMythicAbilityActionSnapshot>(); Parent->ActionId=TEXT("ParentDirectHit");
    Parent->ActionFacets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Reach.Ranged")));
    int32 AppliedStatuses=0;
    const auto StatusEvent=FGameplayTag::RequestGameplayTag(TEXT("Event.ApplyStatus"));
    const auto Listener=ASC->GenericGameplayEventCallbacks.FindOrAdd(StatusEvent).AddLambda([&](const FGameplayEventData* Applied) {
        ++AppliedStatuses;
        const auto* Child=FMythicGameplayEffectContext::ExtractEffectContext(Applied->ContextHandle);
        TestTrue(TEXT("status-only talent preserves source activation and advances reaction depth"),Child && Child->SourceActivationId==Parent->SourceActivationId && Child->ReactionGeneration==3);
        TestTrue(TEXT("periodic status owns its payload without inheriting parent action damage or facets"),Child && Child->DamageOrigin==EMythicDamageOrigin::Periodic && !Child->ActionSnapshot && Child->ActionId.IsNone() && Child->ActionFacets.IsEmpty());
    });
    ON_SCOPE_EXIT { ASC->GenericGameplayEventCallbacks.FindOrAdd(StatusEvent).Remove(Listener); };
    ASC->HandleGameplayEvent(GAS_EVENT_KILL,&Event);
    const auto Times=ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Slow->GrantedStateTag.GetSingleTagContainer()));
    TestTrue(TEXT("a real talent event applies its authored duration instead of the status default"),
        Times.ContainsByPredicate([](float Seconds){return Seconds>1.2f && Seconds<=1.251f;}));
    TestEqual(TEXT("status-only talent emitted exactly one admitted application"),AppliedStatuses,1);
    ASC->RemoveActiveEffectsWithGrantedTags(Slow->GrantedStateTag.GetSingleTagContainer());
    Parent->ReactionGeneration=4;
    ASC->HandleGameplayEvent(GAS_EVENT_KILL,&Event);
    TestEqual(TEXT("a depth-four parent cannot restart a status-only reaction chain"),AppliedStatuses,1);
    TestFalse(TEXT("depth rejection applies no status"),ASC->HasMatchingGameplayTag(Slow->GrantedStateTag));
    ASC->ClearAbility(Handle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityActionSnapshotTest,"Mythic.Combat.Abilities.ActionSnapshotAndSettlement",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityActionSnapshotTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* Source=F.Combatant(); auto* Target=F.Combatant();
    auto* GE=NewObject<UGameplayEffect>(); GE->DurationPolicy=EGameplayEffectDurationType::Instant;
    GE->Executions.AddDefaulted_GetRef().CalculationClass=UMythicDamageApplication::StaticClass();
    Source->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute(),1);
    auto Sealed=MakeShared<FMythicAbilityActionSource>();
    GetDefault<UMythicDamageApplication>()->CaptureActionAttributes(Source,*Sealed);
    GetDefault<UMythicDamageCalculation>()->CaptureActionAttributes(Source,*Sealed);
    auto Action=MakeShared<FMythicAbilityActionSnapshot>(); Action->Source=Sealed; Action->ActionId=TEXT("TestAction"); Action->RolledDamage=100;
    auto MakeContext=[&]() {
        auto Handle=Source->MakeEffectContext(); auto* C=FMythicGameplayEffectContext::ExtractEffectContext(Handle);
        C->ActionSnapshot=Action; C->ActionId=Action->ActionId; C->ActionGroupId=FGuid::NewGuid(); C->ActionOccurrenceId=FGuid::NewGuid();
        C->DamageOrigin=EMythicDamageOrigin::Skill;
        return Handle;
    };
    Source->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),9000);
    Source->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute(),50);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetIncomingDamageMultiplierAttribute(),.5f);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetMaxShieldAttribute(),100);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(),30);
    auto First=MakeContext(); Source->ApplyGameplayEffectToTarget(GE,Target,1,First);
    const auto* C=FMythicGameplayEffectContext::ExtractEffectContext(First);
    TestTrue(TEXT("authored action is accepted"),C->ResolvedDamage.bAccepted);
    TestEqual(TEXT("weapon and postcommit offensive gear do not replace sealed action damage"),C->ResolvedDamage.Attempted,50.0f);
    TestEqual(TEXT("current target shield absorbs its exact share"),C->ResolvedDamage.ShieldAbsorbed,30.0f);
    TestEqual(TEXT("the remaining action damage reaches health"),C->ResolvedDamage.HealthLost,20.0f);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetIncomingDamageMultiplierAttribute(),1);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),10);
    auto DeathPre=Target->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).AddLambda([&](const FGameplayEventData*) {
        Target->ApplyGameplayEffectToSelf(AttributeEffect(UMythicAttributeSet_Life::GetHealingAttribute(),1),1,Target->MakeEffectContext());
    });
    auto Second=MakeContext(); Source->ApplyGameplayEffectToTarget(GE,Target,1,Second);
    const auto* D=FMythicGameplayEffectContext::ExtractEffectContext(Second);
    TestEqual(TEXT("recipient mitigation is read at impact"),D->ResolvedDamage.Attempted,100.0f);
    TestEqual(TEXT("overkill records actual health loss before the separate death-pre heal"),D->ResolvedDamage.HealthLost,10.0f);
    TestFalse(TEXT("death prevention is not reported as a kill"),D->ResolvedDamage.bKilled);
    TestEqual(TEXT("death-pre heal remains real health"),Target->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()),1.0f);
    Target->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).Remove(DeathPre);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),10);
    bool bHealed=false; float NestedGain=0;
    const auto OnHealth=Target->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).AddLambda([&](const FOnAttributeChangeData& Change) {
        if (bHealed || Change.NewValue>0) return;
        bHealed=true; auto HealingContext=Target->MakeEffectContext();
        Target->ApplyGameplayEffectToSelf(AttributeEffect(UMythicAttributeSet_Life::GetHealingAttribute(),1),1,HealingContext);
        NestedGain=FMythicGameplayEffectContext::ExtractEffectContext(HealingContext)->ResolvedHealing.Gained;
    });
    auto Third=MakeContext(); Source->ApplyGameplayEffectToTarget(GE,Target,1,Third);
    Target->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).Remove(OnHealth);
    const auto* E=FMythicGameplayEffectContext::ExtractEffectContext(Third);
    TestTrue(TEXT("health-change reentry was exercised"),bHealed);
    TestEqual(TEXT("attribute callbacks cannot overwrite the outer health-loss receipt"),E->ResolvedDamage.HealthLost,10.0f);
    TestEqual(TEXT("nested healing keeps its own gain"),NestedGain,1.0f);
    TestFalse(TEXT("health-change death prevention does not publish a kill"),E->ResolvedDamage.bKilled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityActionContextTest,"Mythic.Combat.Abilities.ActionContextReplication",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityActionContextTest::RunTest(const FString&) {
    FMythicGameplayEffectContext Source,Loaded;
    Source.ActionId=TEXT("TestAction"); Source.ActionGroupId=FGuid::NewGuid(); Source.ActionOccurrenceId=FGuid::NewGuid();
    Source.ActionFacets.AddTag(GAS_ABILITY_TYPE_SKILL); Source.SourceActivationId=FGuid::NewGuid(); Source.DamageOrigin=EMythicDamageOrigin::Skill;
    Source.ActionSnapshot=MakeShared<FMythicAbilityActionSnapshot>();
    Source.ResolvedDamage.HealthLost=42; Source.ActionShare=.5f;
    const auto RoundTrip=[&](FMythicGameplayEffectContext& Value) {
        FBitWriter Writer(8192,true); bool Saved=false; Value.NetSerialize(Writer,nullptr,Saved);
        FBitReader Reader(Writer.GetData(),Writer.GetNumBits()); bool Read=false; Loaded.NetSerialize(Reader,nullptr,Read);
        return Saved && Read && !Reader.IsError();
    };
    TestTrue(TEXT("action metadata serializes"),RoundTrip(Source));
    TestEqual(TEXT("local action name survives the bit archive"),Loaded.ActionId,Source.ActionId);
    TestTrue(TEXT("activation, group and occurrence identity survive"),Loaded.SourceActivationId==Source.SourceActivationId && Loaded.ActionGroupId==Source.ActionGroupId && Loaded.ActionOccurrenceId==Source.ActionOccurrenceId);
    TestTrue(TEXT("truthful action facets survive"),Loaded.ActionFacets==Source.ActionFacets);
    TestFalse(TEXT("private source snapshot never replicates"),Loaded.ActionSnapshot.IsValid());
    TestEqual(TEXT("server settlement never becomes a client work receipt"),Loaded.ResolvedDamage.HealthLost,0.0f);
    TestEqual(TEXT("server work share never replicates"),Loaded.ActionShare,1.0f);
    Loaded.ActionSnapshot=Source.ActionSnapshot; Loaded.ResolvedDamage.HealthLost=42;
    FMythicGameplayEffectContext Empty;
    TestTrue(TEXT("absent action serializes into a reused destination"),RoundTrip(Empty));
    TestTrue(TEXT("stale metadata and server pointers reset"),Loaded.ActionId.IsNone() && !Loaded.ActionGroupId.IsValid() && Loaded.ActionFacets.IsEmpty() && !Loaded.ActionSnapshot);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityNestedKillTest,"Mythic.Combat.Abilities.NestedKillOwnership",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityNestedKillTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* Source=F.Combatant(); auto* Target=F.Combatant();
    auto* GE=NewObject<UGameplayEffect>(); GE->DurationPolicy=EGameplayEffectDurationType::Instant;
    GE->Executions.AddDefaulted_GetRef().CalculationClass=UMythicDamageApplication::StaticClass();
    auto Sealed=MakeShared<FMythicAbilityActionSource>(); GetDefault<UMythicDamageApplication>()->CaptureActionAttributes(Source,*Sealed);
    const auto MakeContext=[&](float Amount) {
        auto Handle=Source->MakeEffectContext(); auto* C=FMythicGameplayEffectContext::ExtractEffectContext(Handle);
        auto Snapshot=MakeShared<FMythicAbilityActionSnapshot>(); Snapshot->Source=Sealed; Snapshot->RolledDamage=Amount;
        C->ActionSnapshot=Snapshot; return Handle;
    };
    Target->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),10);
    int32 Deaths=0,Kills=0; bool Nested=false; auto Child=MakeContext(10);
    const auto D=Target->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH).AddLambda([&](const FGameplayEventData*){++Deaths;});
    const auto K=Source->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_KILL).AddLambda([&](const FGameplayEventData*){++Kills;});
    const auto H=Target->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).AddLambda([&](const FOnAttributeChangeData& Change) {
        if (Nested || Change.NewValue!=5) return;
        Nested=true; Source->ApplyGameplayEffectToTarget(GE,Target,1,Child);
    });
    auto Parent=MakeContext(5); Source->ApplyGameplayEffectToTarget(GE,Target,1,Parent);
    Target->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).Remove(H);
    Target->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH).Remove(D);
    Source->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_KILL).Remove(K);
    const auto* A=FMythicGameplayEffectContext::ExtractEffectContext(Parent); const auto* B=FMythicGameplayEffectContext::ExtractEffectContext(Child);
    TestTrue(TEXT("a separate lethal child hit ran inside the health change"),Nested);
    TestEqual(TEXT("parent owns only its actual health loss"),A->ResolvedDamage.HealthLost,5.0f);
    TestFalse(TEXT("parent does not claim the child's kill"),A->ResolvedDamage.bKilled);
    TestEqual(TEXT("child owns the remaining health loss"),B->ResolvedDamage.HealthLost,5.0f);
    TestTrue(TEXT("child owns the lethal transition"),B->ResolvedDamage.bKilled);
    TestEqual(TEXT("death is published exactly once"),Deaths,1);
    TestEqual(TEXT("kill is published exactly once"),Kills,1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityActionRecipientConditionsTest,"Mythic.Combat.Abilities.ActionRecipientConditions",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityActionRecipientConditionsTest::RunTest(const FString&) {
    FAbilityCombatFixture F; auto* Source=F.Combatant(); auto* Target=F.Combatant();
    const auto Ranged=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Reach.Ranged"));
    const auto Outgoing=UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute();
    auto* Talent=AttributeEffect(Outgoing,.75f,EGameplayEffectDurationType::Infinite);
    Talent->Modifiers[0].SourceTags.RequireTags.AddTag(Ranged);
    Talent->Modifiers[0].SourceTags.RequireTags.AddTag(GAS_ABILITY_TYPE_SKILL);
    Talent->Modifiers[0].TargetTags.RequireTags.AddTag(GAS_STATE_HEALTH_CRITICAL);
    const auto TalentHandle=Source->ApplyGameplayEffectToSelf(Talent,1,Source->MakeEffectContext());
    auto* ProcTalent=AttributeEffect(UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute(),.75f,EGameplayEffectDurationType::Infinite);
    ProcTalent->Modifiers[0].TargetTags.RequireTags.AddTag(GAS_STATE_HEALTH_CRITICAL);
    auto& Burn=ProcTalent->Modifiers.AddDefaulted_GetRef(); Burn=ProcTalent->Modifiers[0];
    Burn.Attribute=UMythicAttributeSet_Offense::GetApplyBurnOnHitChanceAttribute();
    const auto ProcHandle=Source->ApplyGameplayEffectToSelf(ProcTalent,1,Source->MakeEffectContext());
    auto Sealed=MakeShared<FMythicAbilityActionSource>(); Source->GetOwnedGameplayTags(Sealed->StateTags);
    GetDefault<UMythicDamageApplication>()->CaptureActionAttributes(Source,*Sealed);
    GetDefault<UMythicDamageCalculation>()->CaptureActionAttributes(Source,*Sealed);
    auto Snapshot=MakeShared<FMythicAbilityActionSnapshot>(); Snapshot->Source=Sealed; Snapshot->RolledDamage=100;
    Snapshot->Facets.AddTag(Ranged); for (float& Roll:Snapshot->IntentRolls) Roll=.4f;
    // Changing the caster after commitment cannot change the saved cast, including its conditional modifiers.
    Source->RemoveActiveGameplayEffect(TalentHandle); Source->RemoveActiveGameplayEffect(ProcHandle);
    Source->SetNumericAttributeBase(Outgoing,50);
    const TWeakObjectPtr<UGameplayEffect> OriginalTalent=Talent,OriginalProc=ProcTalent;
    Talent=nullptr; ProcTalent=nullptr;
    TStrongObjectPtr<UGameInstance> KeepFixture(F.Game);
    CollectGarbage(RF_NoFlags);
    TestTrue(TEXT("the saved cast retains conditional definitions after effect removal and garbage collection"),OriginalTalent.IsValid() && OriginalProc.IsValid());
    auto* Hit=NewObject<UGameplayEffect>(); Hit->DurationPolicy=EGameplayEffectDurationType::Instant;
    Hit->Executions.AddDefaulted_GetRef().CalculationClass=UMythicDamageApplication::StaticClass();
    Target->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(),10000);
    Target->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),10000);
    const auto Strike=[&]() {
        auto Context=Source->MakeEffectContext(); auto* C=FMythicGameplayEffectContext::ExtractEffectContext(Context);
        C->ActionSnapshot=Snapshot; C->ActionFacets=Snapshot->Facets; C->DamageOrigin=EMythicDamageOrigin::Skill;
        C->SetCriticalHit(false); C->SetBurn(false); // Isolate the target-qualified outgoing modifier below.
        Source->ApplyGameplayEffectToTarget(Hit,Target,1,Context);
        return C->ResolvedDamage.HealthLost;
    };
    TestEqual(TEXT("healthy recipient excludes the saved conditional talent"),Strike(),100.0f);
    Target->AddLooseGameplayTag(GAS_STATE_HEALTH_CRITICAL);
    TestEqual(TEXT("current recipient tags qualify the talent removed from the caster after capture"),Strike(),175.0f);
    Target->RemoveLooseGameplayTag(GAS_STATE_HEALTH_CRITICAL);
    TestEqual(TEXT("another healthy impact does not inherit previous recipient qualification"),Strike(),100.0f);
    FMythicGameplayEffectContext Intents; Intents.ActionSnapshot=Snapshot;
    FGameplayTagContainer Healthy,Critical(GAS_STATE_HEALTH_CRITICAL);
    UMythicDamageCalculation::ResolveActionIntents(Intents,&Healthy);
    TestFalse(TEXT("healthy recipient does not qualify the saved critical chance"),Intents.IsCriticalHit());
    TestFalse(TEXT("healthy recipient does not qualify the saved burn chance"),Intents.IsBurn());
    UMythicDamageCalculation::ResolveActionIntents(Intents,&Critical);
    TestTrue(TEXT("the fixed group roll qualifies against a critical recipient"),Intents.IsCriticalHit() && Intents.IsBurn());
    UMythicDamageCalculation::ResolveActionIntents(Intents,&Healthy);
    TestFalse(TEXT("calculated proc decisions do not become overrides for the next recipient"),Intents.IsCriticalHit() || Intents.IsBurn());
    Intents.SetCriticalHit(true); Intents.SetBurn(false);
    UMythicDamageCalculation::ResolveActionIntents(Intents,&Critical);
    TestTrue(TEXT("a rune's explicit critical override is preserved"),Intents.IsCriticalHit());
    TestFalse(TEXT("an explicit proc veto is preserved"),Intents.IsBurn());
    UMythicDamageCalculation::ResolveActionIntents(Intents,&Healthy);
    TestTrue(TEXT("guaranteed critical hit is independent of target-conditioned random chance"),Intents.IsCriticalHit());
    return true;
}
#endif
