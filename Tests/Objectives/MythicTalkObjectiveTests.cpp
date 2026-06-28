// Mythic — talk-to-NPC objective emit-gate unit test.
// Covers the shared pure gate (MythicObjectiveEvents::ShouldEmitObjectiveEvent) the talk-to-NPC emit uses to decide
// whether to fire GAS.Event.TalkedToNPC. The live dialogue → event → objective-advance path is server-driven and
// PIE-verified. Run via: Session Frontend → Automation → Mythic.Objectives.TalkEventGate

#include "Misc/AutomationTest.h"
#include "Objectives/MythicObjectiveEvents.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTalkObjectiveGateTest,
    "Mythic.Objectives.TalkEventGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTalkObjectiveGateTest::RunTest(const FString &Parameters) {
    using namespace MythicObjectiveEvents;

    // ShouldEmitObjectiveEvent(bServerAuthoritativeForPlayer, bValidPayloadTag)
    TestTrue(TEXT("server-authoritative + valid payload tag → emit"), ShouldEmitObjectiveEvent(true, true));
    TestFalse(TEXT("not server-authoritative → no emit"), ShouldEmitObjectiveEvent(false, true));
    TestFalse(TEXT("invalid/empty payload tag (NPC isn't a talk target) → no emit"), ShouldEmitObjectiveEvent(true, false));
    TestFalse(TEXT("neither → no emit"), ShouldEmitObjectiveEvent(false, false));

    return true;
}
