#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicTags_GAS.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityActionAdmissionTest,"Mythic.Combat.Abilities.ActionAdmissionBeforePayment",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityActionAdmissionTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    if (!TestNotNull(TEXT("production player avatar initializes"),InitializeAbilityTestAvatar(Player))) return false;
    auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    auto* Affixes=Player->FindComponentByClass<UMythicAffixApplicationComponent>();
    if (!TestNotNull(TEXT("production player owns the equipment transaction component"),Affixes)) return false;
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    RegisterAbilityTestDefinition(Skills,Definition,true,false); Skills->ServerEquipSkill(0,Definition);
    auto* Spec=FindAbilityTestSpec(ASC,Definition);
    if (!TestNotNull(TEXT("probe was granted through the equipped skill component"),Spec)) return false;
    auto* Window=Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance());
    if (!TestNotNull(TEXT("native probe retains the production window implementation"),Window)) return false;
    int32 Begins=0;
    const auto Begin=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_BEGIN).AddLambda([&](const FGameplayEventData*) { ++Begins; });
    ON_SCOPE_EXIT { ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_BEGIN).Remove(Begin); };
    for (const bool bQuarantined:{false,true}) {
        // Inject each transaction failure precondition; activation and payment remain the actual GAS path.
        TGuardValue<bool> Reconcile(Affixes->bReconciliationInProgress,!bQuarantined);
        TGuardValue<bool> Quarantine(Affixes->bApplicationQuarantined,bQuarantined);
        ASC->TryActivateAbility(Spec->Handle);
        TestFalse(TEXT("unsafe source capture ends the attempted cast"),Window->IsWindowAuthority() || Spec->IsActive());
        TestEqual(TEXT("failed capture spends no stamina"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),100.0f);
        TestEqual(TEXT("failed capture starts no cooldown"),Skills->GetCooldownRemaining(Definition),0.0f);
        TestEqual(TEXT("failed capture awards no practice"),Skills->GetSkillUses(Definition),0);
        TestEqual(TEXT("failed capture publishes no begin event"),Begins,0);
    }
    {
        TGuardValue<bool> CancelInValidation(CastChecked<UMythicAbilityWindowTestProbe>(Window)->bCancelDuringValidation,true);
        ASC->TryActivateAbility(Spec->Handle);
        TestFalse(TEXT("validation callback can cancel before payment"),Window->IsWindowAuthority() || Spec->IsActive());
        TestEqual(TEXT("cancelled validation pays no stamina"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),100.0f);
        TestEqual(TEXT("cancelled validation starts no cooldown"),Skills->GetCooldownRemaining(Definition),0.0f);
        TestEqual(TEXT("cancelled validation awards no use"),Skills->GetSkillUses(Definition),0);
        TestEqual(TEXT("cancelled validation emits no begin"),Begins,0);
    }
    TestTrue(TEXT("same real cast activates after the source state becomes safe"),ASC->TryActivateAbility(Spec->Handle));
    TestTrue(TEXT("healthy control commits through production window"),Window->IsWindowAuthority());
    TestEqual(TEXT("healthy control pays the authored native stamina cost exactly once"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),90.0f);
    TestTrue(TEXT("healthy control starts cooldown"),Skills->GetCooldownRemaining(Definition)>0);
    TestEqual(TEXT("healthy control records one practice use"),Skills->GetSkillUses(Definition),1);
    TestEqual(TEXT("healthy control publishes one begin"),Begins,1);
    TestTrue(TEXT("captured action can now be created"),Window->BeginAbilityAction(TEXT("Probe")).IsValid());
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    return true;
}
#endif
