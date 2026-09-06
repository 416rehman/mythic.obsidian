#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Rewards/AbilityReward.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityRewardReadinessTest,"Mythic.Progression.Skills.RewardDurabilityBeforeCatalogue",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityRewardReadinessTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    auto* PC=Game->GetWorld()->SpawnActor<APlayerController>(); PC->PlayerState=Player;
    auto* A=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    auto* B=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.ObsidianMaw")));
    auto& Refinement=B->Modifiers.AddDefaulted_GetRef(); Refinement.ModifierId=TEXT("HeavierImpact");
    Refinement.ActionOverrides=B->Actions; Refinement.ActionOverrides[0].MinimumDamage=Refinement.ActionOverrides[0].MaximumDamage=15;
    auto* Rejected=NewObject<UMythicSkillDefinition>(Game);
    auto* Catalog=Skills->GetCatalog(); FMythicAbilityCatalogTestAccess::BeginPending(Catalog,{A,B});
    auto* Reward=NewObject<UAbilityReward>(Game); Reward->SkillDefinition=A; FRewardContext Context(PC);
    TestTrue(TEXT("a configured earned reward is durably accepted before catalogue readiness"),Reward->Give(Context));
    TestTrue(TEXT("reward replay before readiness is idempotent"),Reward->Give(Context));
    TestEqual(TEXT("pending reward is already exportable once"),Skills->GetSkillProgress().Num(),1);
    TestTrue(TEXT("the exported reward has durable learned ownership"),Skills->GetSkillProgress()[0].bLearned);
    Reward->SkillDefinition=Rejected;
    TestFalse(TEXT("unconfigured reward cannot use pending intake to bypass catalogue membership"),Reward->Give(Context));
    FMythicSkillProgress Saved; Saved.Skill=B; Saved.bLearned=false; Saved.SchemaVersion=0; Saved.ActiveModifiers={0}; Saved.Uses=19; Saved.Level=2;
    Skills->RestoreSkillProgress({Saved},2);
    Skills->RestoreSkills({B,A},2);
    FMythicAbilityCooldownSave Debt; Debt.Skill=FSoftObjectPath(B); Debt.RemainingSeconds=20;
    Skills->RestoreAbilityCooldowns({Debt});
    Reward->SkillDefinition=B;
    TestTrue(TEXT("replay merges with a restored pending row"),Reward->Give(Context));
    const auto EarlySave=Skills->GetSkillProgress(); const auto EarlySlots=Skills->GetEquippedSkillsForSave(); const auto EarlyDebt=Skills->CaptureAbilityCooldowns();
    TestEqual(TEXT("saving before readiness retains reward and restored progress without duplicate rows"),EarlySave.Num(),2);
    const auto* SavedB=EarlySave.FindByPredicate([&](const auto& Row) { return Row.Skill.Get()==B; });
    TestTrue(TEXT("nonzero practice and mastery survive pending restore export"),SavedB && SavedB->Uses==19 && SavedB->Level==2 && SavedB->bLearned);
    TestTrue(TEXT("four sockets retain desired Q/E and empty R/T while no specs are granted"),
        EarlySlots.Num()==4 && EarlySlots[0]==B && EarlySlots[1]==A
        && EarlySlots[2].IsNull() && EarlySlots[3].IsNull() && ASC->GetActivatableAbilities().IsEmpty());
    TestTrue(TEXT("cooldown debt remains exportable during loading"),EarlyDebt.Num()==1 && EarlyDebt[0].RemainingSeconds>19 && EarlyDebt[0].RemainingSeconds<=20);
    TestFalse(TEXT("pending state is not usable gameplay"),Skills->IsReady());
    // A second character restores exactly the data a save-and-exit would have written before readiness.
    auto* Reloaded=Game->GetWorld()->SpawnActor<AMythicPlayerState>();
    InitializeAbilityTestPlayer(Reloaded,false);
    auto* ReloadSkills=Reloaded->GetSkillComponent();
    ReloadSkills->RestoreSkillProgress(EarlySave,2); ReloadSkills->RestoreSkills(EarlySlots,2); ReloadSkills->RestoreAbilityCooldowns(EarlyDebt);
    TestEqual(TEXT("an immediate second save before readiness still retains both learned rows"),ReloadSkills->GetSkillProgress().Num(),2);
    FMythicAbilityCatalogTestAccess::Complete(Catalog,{A,B});
    Skills->SetRestoring(false); ReloadSkills->SetRestoring(false);
    TestTrue(TEXT("validated catalogue activates both accepted entitlements"),Skills->IsSkillUnlocked(A) && Skills->IsSkillUnlocked(B));
    TestEqual(TEXT("restored practice remains exact after catalogue materialization"),Skills->GetSkillUses(B),19);
    TestTrue(TEXT("reward acceptance preserves legacy index-to-ID refinement migration"),Skills->GetActiveModifiers(B).Contains(0));
    TestTrue(TEXT("reloaded early save retains earned ownership"),ReloadSkills->IsSkillUnlocked(A) && ReloadSkills->IsSkillUnlocked(B));
    TestEqual(TEXT("readiness grants exactly two selected specs"),ASC->GetActivatableAbilities().Num(),2);
    Catalog->OnReady.Broadcast();
    TestEqual(TEXT("duplicate readiness cannot duplicate progress"),Skills->GetSkillProgress().Num(),2);
    TestEqual(TEXT("duplicate readiness cannot duplicate specs"),ASC->GetActivatableAbilities().Num(),2);
    TestTrue(TEXT("debt survives materialization"),Skills->GetCooldownRemaining(B)>0);
    return true;
}
#endif
