#include "Misc/AutomationTest.h"
#include "Containers/Set.h"
#include "World/Digging/MythicDigSiteRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDigSiteTest,
    "Mythic.World.DigSite",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDigSiteTest::RunTest(const FString &Parameters) {
    namespace S = MythicDigSite;

    const FVector Site(1000.0f, 2000.0f, 0.0f);
    const float Tol = 300.0f;
    TestTrue(TEXT("standing exactly on the site → at site"), S::IsAtDigSite(Site, Site, Tol));
    TestTrue(TEXT("within tolerance → at site"), S::IsAtDigSite(Site + FVector(200.0f, 0.0f, 0.0f), Site, Tol));
    TestTrue(TEXT("exactly at tolerance → at site (inclusive)"), S::IsAtDigSite(Site + FVector(300.0f, 0.0f, 0.0f), Site, Tol));
    TestFalse(TEXT("beyond tolerance → not at site"), S::IsAtDigSite(Site + FVector(301.0f, 0.0f, 0.0f), Site, Tol));
    TestFalse(TEXT("zero tolerance → never at site (disabled)"), S::IsAtDigSite(Site, Site, 0.0f));

    TSet<int32> Authored;
    Authored.Add(1);
    Authored.Add(7);
    TestTrue(TEXT("authored id → resolves"), S::ResolveDigSite(Authored, 7));
    TestFalse(TEXT("unknown id → does not resolve"), S::ResolveDigSite(Authored, 99));
    TestFalse(TEXT("INDEX_NONE → does not resolve"), S::ResolveDigSite(Authored, INDEX_NONE));

    TestFalse(TEXT("empty ground (no site) → no yield"), S::ShouldYieldBuriedFind( false, false, false));
    TestFalse(TEXT("site exists but digger not on it → no yield"), S::ShouldYieldBuriedFind(true, false, false));

    TSet<int32> Consumed;
    const int32 DugSite = 7;
    const bool bFirstDig = S::ShouldYieldBuriedFind( true, true, Consumed.Contains(DugSite));
    TestTrue(TEXT("first dig at an un-consumed site → yields"), bFirstDig);
    Consumed.Add(DugSite);
    const bool bSecondDig = S::ShouldYieldBuriedFind( true, true, Consumed.Contains(DugSite));
    TestFalse(TEXT("second dig at a consumed site → no yield (permanent one-shot)"), bSecondDig);

    return true;
}
