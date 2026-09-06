#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityNoTimedCooldownTest,"Mythic.Combat.Abilities.NoTimedCooldown",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityNoTimedCooldownTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    if (!TestNotNull(TEXT("real local avatar initializes"),InitializeAbilityTestAvatar(Player))) return false;
    auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    Definition->ReusePolicy=EMythicAbilityReusePolicy::NoTimedCooldown;
    TArray<FText> Errors;
    TestFalse(TEXT("no-timed policy rejects contradictory authored cooldown data"),Definition->ValidateDefinition(Errors));
    Definition->CooldownSeconds=0; Definition->CooldownTag={}; Definition->CooldownEffect=nullptr;
    Errors.Reset(); TestTrue(TEXT("resource-gated definition requires no dummy cooldown effect or tag"),Definition->ValidateDefinition(Errors));
    RegisterAbilityTestDefinition(Skills,Definition,true,false); Skills->ServerEquipSkill(0,Definition);
    auto* Spec=FindAbilityTestSpec(ASC,Definition);
    if (!TestNotNull(TEXT("definition grants through the ordinary socket"),Spec)) return false;
    auto* Window=Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance());
    if (!TestNotNull(TEXT("production window exists"),Window)) return false;
    for (int32 I=0; I<2; ++I) {
        TestTrue(TEXT("next input can commit immediately after the prior activation ends"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
        TestEqual(TEXT("each activation still pays its ordinary stamina cost"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),100.0f-10.0f*(I+1));
        TestEqual(TEXT("accepted uses remain independent of cooldown policy"),Skills->GetSkillUses(Definition),I+1);
        TestFalse(TEXT("no timed debt does not permit retriggering an active instance"),ASC->TryActivateAbility(Spec->Handle));
        TestEqual(TEXT("active-instance rejection spends no additional stamina"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),100.0f-10.0f*(I+1));
        TestEqual(TEXT("active-instance rejection awards no practice"),Skills->GetSkillUses(Definition),I+1);
        TestEqual(TEXT("no hidden minimum cooldown is synthesized"),Skills->GetCooldownRemaining(Definition),0.0f);
        TestTrue(TEXT("no timed debt is exported"),Skills->CaptureAbilityCooldowns().IsEmpty());
        Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    }
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),0);
    TestFalse(TEXT("ordinary resource cost still rejects activation"),ASC->TryActivateAbility(Spec->Handle));
    TestEqual(TEXT("insufficient resources award no practice"),Skills->GetSkillUses(Definition),2);
    TestFalse(TEXT("insufficient resources open no window"),Window->IsWindowAuthority());
    TestFalse(TEXT("restoration cannot manufacture timed debt for this policy"),Skills->StartCooldown(Definition,12));
    // A stale save row can arrive before definitions validate. It must stop gating/exporting after policy resolution.
    FMythicAbilityCatalogTestAccess::BeginPending(Skills->GetCatalog(),{Definition});
    FMythicAbilityCooldownSave Debt; Debt.Skill=FSoftObjectPath(Definition); Debt.RemainingSeconds=12;
    Skills->RestoreAbilityCooldowns({Debt});
    TestEqual(TEXT("unresolved saved debt is preserved while catalogue loading"),Skills->CaptureAbilityCooldowns().Num(),1);
    FMythicAbilityCatalogTestAccess::Complete(Skills->GetCatalog(),{});
    TestEqual(TEXT("a ready catalogue that rejects the definition does not erase its unresolved debt"),Skills->CaptureAbilityCooldowns().Num(),1);
    FMythicAbilityCatalogTestAccess::Complete(Skills->GetCatalog(),{Definition});
    TestEqual(TEXT("retired timed debt does not gate a no-timed definition"),Skills->GetCooldownRemaining(Definition),0.0f);
    TestTrue(TEXT("resolved no-timed policy retires stale debt on save"),Skills->CaptureAbilityCooldowns().IsEmpty());
    return true;
}
#endif
