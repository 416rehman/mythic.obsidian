#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityCooldownDisplayTest,"Mythic.Combat.Abilities.CooldownDisplay",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityCooldownDisplayTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    if (!TestNotNull(TEXT("local production avatar initializes"),InitializeAbilityTestAvatar(Player))) return false;
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Skills=Player->GetSkillComponent();
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    RegisterAbilityTestDefinition(Skills,Definition,true,false);
    auto Apply=[&](float Seconds) {
        auto Spec=ASC->MakeOutgoingSpec(Definition->CooldownEffect,1,ASC->MakeEffectContext());
        Spec.Data->SetDuration(Seconds,true); Spec.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
        return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    // Model overlapping replicated effects without authority's independent monotonic debt map.
    const auto Old=Apply(100); ASC->ModifyActiveEffectStartTime(Old,-95);
    const auto Current=Apply(20);
    float Remaining=0,Duration=0; Skills->GetCooldownDisplay(Definition,Remaining,Duration);
    TestEqual(TEXT("the effect with longest remaining time gates input"),Remaining,20.0f,0.01f);
    TestEqual(TEXT("radial uses that effect's resolved duration, not an old or authored span"),Duration,20.0f,0.01f);
    Definition->ReusePolicy=EMythicAbilityReusePolicy::NoTimedCooldown;
    Skills->GetCooldownDisplay(Definition,Remaining,Duration);
    TestEqual(TEXT("no-timed policy exposes no fabricated remainder"),Remaining,0.0f);
    TestEqual(TEXT("no-timed policy exposes no radial duration"),Duration,0.0f);
    ASC->RemoveActiveGameplayEffect(Old); ASC->RemoveActiveGameplayEffect(Current);
    return true;
}
#endif
