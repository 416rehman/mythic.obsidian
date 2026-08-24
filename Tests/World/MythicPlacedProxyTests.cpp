#include "Misc/AutomationTest.h"

#include "World/Placement/MythicPlacedProxyTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPlacedProxyTest,
    "Mythic.World.PlacedProxy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPlacedProxyTest::RunTest(const FString &Parameters) {
    using Rules = FMythicPlacedProxyRules;

    // Distances are squared; promote at 100, demote at 200 => PromoteSq 10000, DemoteSq 40000.
    const float Promote = 100.0f;
    const float Demote = 200.0f;
    const float InsidePromote = 70.0f * 70.0f; // 4900 < 10000
    const float InBand = 150.0f * 150.0f;       // 22500: past promote, inside demote
    const float BeyondDemote = 250.0f * 250.0f; // 62500 > 40000

    // Cold start: only inside the promote radius does an unpromoted proxy promote.
    TestTrue(TEXT("inside promote radius, an unpromoted proxy promotes"),
             Rules::ShouldBePromoted(InsidePromote, Promote, Demote, false));
    TestFalse(TEXT("in the band, an unpromoted proxy stays demoted"),
              Rules::ShouldBePromoted(InBand, Promote, Demote, false));

    // Hysteresis: once promoted, it holds through the band and only drops past the demote radius.
    TestTrue(TEXT("in the band, a promoted proxy holds (no flicker)"),
             Rules::ShouldBePromoted(InBand, Promote, Demote, true));
    TestFalse(TEXT("beyond the demote radius, a promoted proxy demotes"),
              Rules::ShouldBePromoted(BeyondDemote, Promote, Demote, true));

    // A negative (invalid) distance never promotes.
    TestFalse(TEXT("a negative distance never promotes"),
              Rules::ShouldBePromoted(-1.0f, Promote, Demote, true));

    // Misconfiguration guard: a demote radius smaller than promote can't invert the band into flicker — the
    // effective demote is clamped up to the promote radius, so promote and demote coincide.
    TestFalse(TEXT("demote < promote cannot create an inverted band"),
              Rules::ShouldBePromoted(InBand, 100.0f, 50.0f, true));

    // The instance is the demoted representation: shown only when a mesh exists and the proxy is not promoted.
    TestTrue(TEXT("a demoted proxy with a mesh shows its instance"), Rules::ShouldHaveInstance(false, true));
    TestFalse(TEXT("a promoted proxy hides its instance"), Rules::ShouldHaveInstance(true, true));
    TestFalse(TEXT("no mesh, no instance"), Rules::ShouldHaveInstance(false, false));

    return true;
}
