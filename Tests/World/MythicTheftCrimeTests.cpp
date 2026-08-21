
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "World/Ownership/MythicOwnership.h"
#include "World/Interactables/MythicToggleable.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_TEST_THEFT_FACTION_A, "Test.Theft.FactionA");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_TEST_THEFT_FACTION_B, "Test.Theft.FactionB");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTheftCrimeTest,
    "Mythic.Crime.Theft",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTheftCrimeTest::RunTest(const FString &Parameters) {
    const FMythicMoralAction Theft = FMythicMoralSignature::MakeTheftActionMoralVector();
    const float TheftAxis = Theft.AxisValues[static_cast<int32>(EMythicMoralAxis::Theft)];
    const float Violence = Theft.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)];

    TestTrue(TEXT("theft vector: Theft axis > 0"), TheftAxis > 0.0f);
    TestEqual(TEXT("theft vector: Violence == 0"), Violence, 0.0f);

    const FMythicMoralAction Trespass = FMythicMoralSignature::MakeTrespassActionMoralVector();
    const float TrespassAuthority = Trespass.AxisValues[static_cast<int32>(EMythicMoralAxis::Authority)];
    const FMythicMoralAction Kill = FMythicMoralSignature::MakeKillActionMoralVector();
    const float KillViolence = Kill.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)];
    TestTrue(TEXT("theft |Theft| > trespass |Authority| (graver than a territorial step)"),
             FMath::Abs(TheftAxis) > FMath::Abs(TrespassAuthority));
    TestTrue(TEXT("theft |Theft| < kill |Violence| (a single lift is not a killing)"),
             FMath::Abs(TheftAxis) < FMath::Abs(KillViolence));

    FMythicIdeologyProfile LawAbiding;
    LawAbiding.GetAxisMutable(EMythicMoralAxis::Theft) = -1.0f;
    const EMythicMoralSeverity SevLaw = FMythicMoralSignature::EvaluateActionSeverity(
        Theft, LawAbiding, 0.2f, 0.5f, 0.8f);
    TestEqual(TEXT("theft vs anti-theft faction → Condemn (criminal charge)"),
              static_cast<int32>(SevLaw), static_cast<int32>(EMythicMoralSeverity::Condemn));

    FMythicIdeologyProfile Permissive;
    Permissive.GetAxisMutable(EMythicMoralAxis::Theft) = 1.0f;
    const EMythicMoralSeverity SevPermissive = FMythicMoralSignature::EvaluateActionSeverity(
        Theft, Permissive, 0.2f, 0.5f, 0.8f);
    TestEqual(TEXT("theft vs permissive faction → Ignore"),
              static_cast<int32>(SevPermissive), static_cast<int32>(EMythicMoralSeverity::Ignore));

    const FGameplayTag FactionA = TAG_TEST_THEFT_FACTION_A;
    const FGameplayTag FactionB = TAG_TEST_THEFT_FACTION_B;
    const FGameplayTag NoFaction;

    FMythicOwnership OwnedByA;
    OwnedByA.OwnerFactionTag = FactionA;
    FMythicOwnership OwnedByNpc;
    OwnedByNpc.OwnerNpcKey = TEXT("npc-merchant-42");
    FMythicOwnership Unowned;

    TestTrue(TEXT("faction stamp → owned"), OwnedByA.IsOwned());
    TestTrue(TEXT("npc-key stamp → owned"), OwnedByNpc.IsOwned());
    TestFalse(TEXT("empty stamp → unowned"), Unowned.IsOwned());

    TestTrue(TEXT("owned + different-faction thief + enabled → submit"),
             MythicTheftCrime::ShouldSubmitTheft(OwnedByA, FactionB, true));
    TestTrue(TEXT("owned + player (no faction) + enabled → submit"),
             MythicTheftCrime::ShouldSubmitTheft(OwnedByA, NoFaction, true));
    TestTrue(TEXT("npc-owned + player + enabled → submit"),
             MythicTheftCrime::ShouldSubmitTheft(OwnedByNpc, NoFaction, true));
    TestFalse(TEXT("unowned → no submit"),
              MythicTheftCrime::ShouldSubmitTheft(Unowned, FactionB, true));
    TestFalse(TEXT("own-faction thief → no submit"),
              MythicTheftCrime::ShouldSubmitTheft(OwnedByA, FactionA, true));
    TestFalse(TEXT("disabled master switch → no submit"),
              MythicTheftCrime::ShouldSubmitTheft(OwnedByA, FactionB, false));

    TestTrue(TEXT("pick: roll below chance succeeds"),
             AMythicToggleable::ResolvePickLock( 5, 5, 0.49f));
    TestFalse(TEXT("pick: roll at/above chance fails"),
              AMythicToggleable::ResolvePickLock(5, 5, 0.5f));
    TestTrue(TEXT("pick: skill advantage raises chance vs parity"),
             AMythicToggleable::ComputePickSuccessChance(10, 5) > AMythicToggleable::ComputePickSuccessChance(5, 5));
    TestTrue(TEXT("pick: difficulty disadvantage lowers chance vs parity"),
             AMythicToggleable::ComputePickSuccessChance(5, 10) < AMythicToggleable::ComputePickSuccessChance(5, 5));

    return true;
}
