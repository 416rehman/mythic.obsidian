
#include "Misc/AutomationTest.h"
#include "GAS/Executions/MythicDamageApplication.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillDamageTest,
    "Mythic.Combat.SkillDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillDamageTest::RunTest(const FString &Parameters) {
    using D = UMythicDamageApplication;

    TestEqual(TEXT("non-skill hit ignores the bonus"), D::ApplySkillDamageBonus(100.0f, false, 0.5f), 100.0f);
    TestEqual(TEXT("skill hit scales by (1+bonus)"), D::ApplySkillDamageBonus(100.0f, true, 0.5f), 150.0f);
    TestEqual(TEXT("zero bonus leaves a skill hit unchanged"), D::ApplySkillDamageBonus(100.0f, true, 0.0f), 100.0f);
    TestEqual(TEXT("negative bonus clamps a skill hit to 0"), D::ApplySkillDamageBonus(100.0f, true, -2.0f), 0.0f);
    TestEqual(TEXT("non-skill hit with a negative bonus is unchanged"), D::ApplySkillDamageBonus(100.0f, false, -2.0f), 100.0f);

    return true;
}
