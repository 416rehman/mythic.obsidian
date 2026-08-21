
#include "Misc/AutomationTest.h"
#include "Containers/Set.h"
#include "Player/MythicPlayerController.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicZoneEntryTest,
    "Mythic.World.ZoneEntry",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicZoneEntryTest::RunTest(const FString &Parameters) {
    TSet<int32> Discovered;
    Discovered.Add(7);
    Discovered.Add(12);

    TestFalse(TEXT("undiscovered settlement → cannot fast travel"),
              AMythicPlayerController::CanFastTravel(Discovered, 99, false));
    TestTrue(TEXT("discovered settlement → can fast travel"),
             AMythicPlayerController::CanFastTravel(Discovered, 7, false));
    TestTrue(TEXT("other discovered settlement → can fast travel"),
             AMythicPlayerController::CanFastTravel(Discovered, 12, false));
    TestFalse(TEXT("INDEX_NONE target → cannot fast travel"),
              AMythicPlayerController::CanFastTravel(Discovered, INDEX_NONE, false));
    TestFalse(TEXT("blocked (combat) → cannot fast travel even if discovered"),
              AMythicPlayerController::CanFastTravel(Discovered, 12, true));
    TestFalse(TEXT("empty discovered set → cannot fast travel"),
              AMythicPlayerController::CanFastTravel(TSet<int32>(), 7, false));

    const FMythicMoralAction Trespass = FMythicMoralSignature::MakeTrespassActionMoralVector();
    const float Authority = Trespass.AxisValues[static_cast<int32>(EMythicMoralAxis::Authority)];
    const float Loyalty = Trespass.AxisValues[static_cast<int32>(EMythicMoralAxis::Loyalty)];
    const float Violence = Trespass.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)];

    TestTrue(TEXT("trespass vector: Authority < 0"), Authority < 0.0f);
    TestTrue(TEXT("trespass vector: Loyalty < 0"), Loyalty < 0.0f);
    TestEqual(TEXT("trespass vector: Violence == 0"), Violence, 0.0f);

    const FMythicMoralAction Kill = FMythicMoralSignature::MakeKillActionMoralVector();
    const float KillViolence = Kill.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)];
    TestTrue(TEXT("trespass |Authority| < kill |Violence| (minor beat)"), FMath::Abs(Authority) < FMath::Abs(KillViolence));
    TestTrue(TEXT("trespass |Authority| <= 0.5 (mild)"), FMath::Abs(Authority) <= 0.5f);
    TestTrue(TEXT("trespass |Loyalty| <= 0.5 (mild)"), FMath::Abs(Loyalty) <= 0.5f);
    TestTrue(TEXT("trespass |Loyalty| <= |Authority| (Loyalty is the secondary axis)"),
             FMath::Abs(Loyalty) <= FMath::Abs(Authority));

    FMythicIdeologyProfile OrderKeeping;
    OrderKeeping.GetAxisMutable(EMythicMoralAxis::Authority) = 1.0f;
    const EMythicMoralSeverity SevOrder = FMythicMoralSignature::EvaluateActionSeverity(
        Trespass, OrderKeeping, 0.2f, 0.5f, 0.8f);
    TestEqual(TEXT("trespass vs order-keeping faction → Disapprove (minor)"),
              static_cast<int32>(SevOrder), static_cast<int32>(EMythicMoralSeverity::Disapprove));

    FMythicIdeologyProfile Anarchic;
    Anarchic.GetAxisMutable(EMythicMoralAxis::Authority) = -1.0f;
    const EMythicMoralSeverity SevAnarchic = FMythicMoralSignature::EvaluateActionSeverity(
        Trespass, Anarchic, 0.2f, 0.5f, 0.8f);
    TestEqual(TEXT("trespass vs anarchic faction → Ignore"),
              static_cast<int32>(SevAnarchic), static_cast<int32>(EMythicMoralSeverity::Ignore));

    return true;
}
