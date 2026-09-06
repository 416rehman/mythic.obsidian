#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Tests/Progression/MythicAchievementRestoreTestReward.h"
#include "Progression/MythicAchievementComponent.h"
#include "Progression/MythicAchievementSet.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAchievementRestoreTest,"Mythic.Progression.Skills.MatureDeedRestore",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAchievementRestoreTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Skill=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.ObsidianMaw")));
    Skill->Acquisition=EMythicAbilityAcquisition::RewardOnly;
    auto* Reward=NewObject<UMythicAchievementRestoreTestReward>(Game); Reward->SkillDefinition=Skill; Reward->Activate=false;
    auto* Deed=NewObject<UMythicAchievementDefinition>(Game);
    Deed->AchievementTag=FGameplayTag::RequestGameplayTag(TEXT("Achievement.Ability.StoneSentence"));
    auto& Requirement=Deed->Condition.StatRequirements.AddDefaulted_GetRef();
    Requirement.StatTag=FGameplayTag::RequestGameplayTag(TEXT("Stat.Kill.Generic")); Requirement.MinValue=25;
    Deed->Reward.AbilityReward=Reward;
    auto* Manual=NewObject<UMythicAchievementDefinition>(Game);
    Manual->AchievementTag=FGameplayTag::RequestGameplayTag(TEXT("Achievement.Delver")); Manual->Reward.AbilityReward=Reward;
    auto* Set=NewObject<UMythicAchievementSet>(Game); Set->Achievements={Deed,Manual};
    for (int64 Count:{24,25}) {
        auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
        auto* PC=Game->GetWorld()->SpawnActor<APlayerController>(); PC->PlayerState=Player; Player->SetOwner(PC);
        auto* Skills=Player->GetSkillComponent(); FMythicAbilityCatalogTestAccess::Add(Skills->GetCatalog(),Skill);
        auto* Achievements=Player->GetAchievementComponent(); Achievements->AchievementSet=Set;
        auto* Ledger=Player->GetStatLedgerComponent(); FMythicStatCounter Counter; Counter.Tag=Requirement.StatTag; Counter.Value=Count;
        Achievements->SetRestoring(true); Ledger->RestoreCharacterCounters({Counter}); Achievements->RestoreUnlockedAchievements({});
        const int32 Before=Reward->GiveCount;
        Achievements->ReconcileSatisfiedAchievements();
        TestEqual(TEXT("reconciliation cannot award while restoration is guarded"),Reward->GiveCount,Before);
        Achievements->SetRestoring(false); Achievements->ReconcileSatisfiedAchievements();
        const bool bEarned=Count==25;
        TestEqual(TEXT("saved threshold determines new deed eligibility without another kill"),Achievements->IsAchievementUnlocked(Deed->AchievementTag),bEarned);
        TestEqual(TEXT("eligible saved counters deliver actual skill ownership"),Skills->IsSkillUnlocked(Skill),bEarned);
        TestEqual(TEXT("eligible deed reward is delivered exactly once"),Reward->GiveCount,Before+(bEarned?1:0));
        TestFalse(TEXT("manual empty definition does not auto-unlock on load"),Achievements->IsAchievementUnlocked(Manual->AchievementTag));
        const auto SavedAchievements=Achievements->GetUnlockedAchievements();
        const auto SavedProgress=Skills->GetSkillProgress();
        Achievements->ReconcileSatisfiedAchievements();
        Achievements->SetRestoring(true); Achievements->RestoreUnlockedAchievements(SavedAchievements);
        Skills->RestoreSkillProgress(SavedProgress,1); Achievements->SetRestoring(false);
        Achievements->ReconcileSatisfiedAchievements(); Skills->SetRestoring(false);
        TestEqual(TEXT("repeat reconciliation and restored earned ledger do not replay rewards"),Reward->GiveCount,Before+(bEarned?1:0));
        TestEqual(TEXT("earned ownership survives progress restore"),Skills->IsSkillUnlocked(Skill),bEarned);
    }
    return true;
}
#endif
