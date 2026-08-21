
#include "Misc/AutomationTest.h"

#include "GameplayEffect.h"

#include "GAS/Executions/MythicCombatRoll.h"

#include "GAS/Abilities/MythicGA_Triggered.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTriggeredProcTest,
    "Mythic.Combat.TriggeredProc",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTriggeredProcTest::RunTest(const FString &Parameters) {
    using GA = UMythicGA_Triggered;

    // Chance, with no cooldown configured.
    TestTrue(TEXT("a certain proc fires on the worst roll"), GA::ShouldProc(1.0f, 0.0f, 100.0, 0.0, 0.999999f));
    TestFalse(TEXT("a zero chance never fires"), GA::ShouldProc(0.0f, 0.0f, 100.0, 0.0, 0.0f));
    TestTrue(TEXT("a 25% proc fires on a roll inside the band"), GA::ShouldProc(0.25f, 0.0f, 100.0, 0.0, 0.20f));
    TestFalse(TEXT("a 25% proc does not fire on a roll outside it"), GA::ShouldProc(0.25f, 0.0f, 100.0, 0.0, 0.30f));

    // Rolled chances come from item affixes, so a stacked one can arrive above 1 exactly as dodge did.
    TestFalse(TEXT("a stacked chance is still bounded, not free"), GA::ShouldProc(8.0f, 5.0f, 101.0, 100.0, 0.0f));

    // Internal cooldown. Without it attack speed alone would carry a proc build.
    TestFalse(TEXT("a proc inside its cooldown does not fire"), GA::ShouldProc(1.0f, 5.0f, 102.0, 100.0, 0.0f));
    TestTrue(TEXT("a proc past its cooldown fires again"), GA::ShouldProc(1.0f, 5.0f, 106.0, 100.0, 0.0f));
    TestTrue(TEXT("a proc exactly at its cooldown fires"), GA::ShouldProc(1.0f, 5.0f, 105.0, 100.0, 0.0f));

    // A clause that has never fired must not be treated as on cooldown at world time zero.
    TestTrue(TEXT("a clause that never fired is not on cooldown"), GA::ShouldProc(1.0f, 5.0f, 0.0, 0.0, 0.0f));

    // Target resolution. Both damage events carry the other party in Target, so one rule covers dealing and taking.
    {
        AActor *Owner = reinterpret_cast<AActor *>(0x1);
        FGameplayEventData Payload;
        FMythicTriggerSpec SelfSpec;
        SelfSpec.Target = EMythicTriggerTarget::Self;
        TestEqual(TEXT("Self resolves the owner"), GA::ResolveTarget(SelfSpec, &Payload, Owner), Owner);

        FMythicTriggerSpec OtherSpec;
        OtherSpec.Target = EMythicTriggerTarget::Other;
        TestNull(TEXT("Other with an empty payload target resolves nothing"), GA::ResolveTarget(OtherSpec, &Payload, Owner));
        TestNull(TEXT("Other with no payload resolves nothing"), GA::ResolveTarget(OtherSpec, nullptr, Owner));
    }

    // Conditions. Every field defaults to "no gate", so a clause that authors none behaves as one without.
    {
        const FGameplayTagContainer Empty;
        FGameplayTagContainer World;
        World.AddTag(FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Snow"), false));
        World.AddTag(FGameplayTag::RequestGameplayTag(FName("Environment.Time.Night"), false));

        FMythicTriggerCondition None;
        TestTrue(TEXT("a default condition is no gate"), GA::PassesCondition(None, World, Empty, Empty, Empty, 1.0f, 1.0f));
        TestTrue(TEXT("a default condition ignores health entirely"), GA::PassesCondition(None, Empty, Empty, Empty, Empty, 0.01f, 0.01f));

        FMythicTriggerCondition Snow;
        Snow.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Snow"), false);
        TestTrue(TEXT("the snow tag is registered, or this whole block proves nothing"), Snow.RequiredWorldTag.IsValid());
        TestTrue(TEXT("a weather gate opens in that weather"), GA::PassesCondition(Snow, World, Empty, Empty, Empty, 1.0f, 1.0f));
        TestFalse(TEXT("a weather gate stays shut with no world state"), GA::PassesCondition(Snow, Empty, Empty, Empty, Empty, 1.0f, 1.0f));

        FMythicTriggerCondition Rain;
        Rain.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Rain"), false);
        TestFalse(TEXT("a rain gate stays shut while it snows"), GA::PassesCondition(Rain, World, Empty, Empty, Empty, 1.0f, 1.0f));

        // Authoring the parent gates on any weather, matching FMythicWeatherDamageMod.
        FMythicTriggerCondition AnyWeather;
        AnyWeather.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather"), false);
        TestTrue(TEXT("the parent tag gates on any weather"), GA::PassesCondition(AnyWeather, World, Empty, Empty, Empty, 1.0f, 1.0f));

        // Proficiency tracks ride on the event payload, which is how a talent keys off fishing not mining.
        FGameplayTagContainer FishingEvent;
        FishingEvent.AddTag(FGameplayTag::RequestGameplayTag(FName("Proficiency.Fishing"), false));

        FMythicTriggerCondition Fishing;
        Fishing.RequiredEventTag = FGameplayTag::RequestGameplayTag(FName("Proficiency.Fishing"), false);
        TestTrue(TEXT("the fishing track is registered, or this block proves nothing"), Fishing.RequiredEventTag.IsValid());
        TestTrue(TEXT("a fishing gate opens on a fishing event"),
                 GA::PassesCondition(Fishing, Empty, FishingEvent, Empty, Empty, 1.0f, 1.0f));
        TestFalse(TEXT("a fishing gate stays shut on an event carrying nothing"),
                  GA::PassesCondition(Fishing, Empty, Empty, Empty, Empty, 1.0f, 1.0f));

        FMythicTriggerCondition Mining;
        Mining.RequiredEventTag = FGameplayTag::RequestGameplayTag(FName("Proficiency.Mining"), false);
        TestFalse(TEXT("a mining gate stays shut on a fishing event"),
                  GA::PassesCondition(Mining, Empty, FishingEvent, Empty, Empty, 1.0f, 1.0f));

        // The parent tag keys off any kind of work at all, which is what Scholar's Edge describes.
        FMythicTriggerCondition AnyWork;
        AnyWork.RequiredEventTag = FGameplayTag::RequestGameplayTag(FName("Proficiency"), false);
        TestTrue(TEXT("the parent track gates on any work"),
                 GA::PassesCondition(AnyWork, Empty, FishingEvent, Empty, Empty, 1.0f, 1.0f));

        // Cornered Beast: "Below half your life, every blow tears deeper."
        FMythicTriggerCondition Cornered;
        Cornered.SourceHealthMax = 0.5f;
        TestTrue(TEXT("a wounded owner passes a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, Empty, 0.4f, 1.0f));
        TestFalse(TEXT("a healthy owner fails a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, Empty, 0.9f, 1.0f));
        TestTrue(TEXT("exactly half passes a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, Empty, 0.5f, 1.0f));

        // Executioner: "Anything already dying takes a killing blow."
        FMythicTriggerCondition Dying;
        Dying.TargetHealthMax = 0.2f;
        TestTrue(TEXT("a dying target passes an execute gate"), GA::PassesCondition(Dying, Empty, Empty, Empty, Empty, 1.0f, 0.15f));
        TestFalse(TEXT("a healthy target fails an execute gate"), GA::PassesCondition(Dying, Empty, Empty, Empty, Empty, 1.0f, 0.5f));

        // Envious Edge: "The first blow against an unbloodied foe bites far deeper."
        FMythicTriggerCondition Unbloodied;
        Unbloodied.TargetHealthMin = 1.0f;
        TestTrue(TEXT("an untouched target passes an unbloodied gate"), GA::PassesCondition(Unbloodied, Empty, Empty, Empty, Empty, 1.0f, 1.0f));
        TestFalse(TEXT("a scratched target fails an unbloodied gate"), GA::PassesCondition(Unbloodied, Empty, Empty, Empty, Empty, 1.0f, 0.99f));

        // An actor with no health reads as full, so a below-half gate cannot fire on something that cannot bleed.
        TestFalse(TEXT("something with no health fails a below-half gate"),
                  GA::PassesCondition(Cornered, Empty, Empty, Empty, Empty, GA::GetHealthFraction(nullptr), 1.0f));
    }

    // Every-Nth counting. "Every third strike falls like a hammer" must land on 3, 6, 9 and nothing between.
    {
        TestTrue(TEXT("no cadence fires on the first event"), GA::IsNthEvent(0, 1));
        TestTrue(TEXT("a cadence of one fires on every event"), GA::IsNthEvent(1, 7));

        TestFalse(TEXT("third strike does not fire on the first"), GA::IsNthEvent(3, 1));
        TestFalse(TEXT("third strike does not fire on the second"), GA::IsNthEvent(3, 2));
        TestTrue(TEXT("third strike fires on the third"), GA::IsNthEvent(3, 3));
        TestFalse(TEXT("third strike does not fire on the fourth"), GA::IsNthEvent(3, 4));
        TestTrue(TEXT("third strike fires again on the sixth"), GA::IsNthEvent(3, 6));

        // A count of zero means nothing has happened yet, which is never the Nth.
        TestFalse(TEXT("a cadence never fires before any event"), GA::IsNthEvent(3, 0));

        TestTrue(TEXT("one blow in five fires on the fifth"), GA::IsNthEvent(5, 5));
        TestFalse(TEXT("one blow in five does not fire on the fourth"), GA::IsNthEvent(5, 4));
    }

    // Facing, measured from the other party's forward vector toward the owner, so Behind means their back is to us.
    {
        const FVector North(1, 0, 0);
        const float Bound = 0.707f;

        TestEqual(TEXT("standing where they are looking is front"),
                  GA::ResolveFacing(North, FVector(1, 0, 0), Bound), EMythicTriggerFacing::Front);
        TestEqual(TEXT("standing at their back is behind"),
                  GA::ResolveFacing(North, FVector(-1, 0, 0), Bound), EMythicTriggerFacing::Behind);
        TestEqual(TEXT("standing off their shoulder is a flank"),
                  GA::ResolveFacing(North, FVector(0, 1, 0), Bound), EMythicTriggerFacing::Flank);
        TestEqual(TEXT("the other shoulder is also a flank"),
                  GA::ResolveFacing(North, FVector(0, -1, 0), Bound), EMythicTriggerFacing::Flank);

        TestEqual(TEXT("just inside the front arc is front"),
                  GA::ResolveFacing(North, FVector(0.8f, 0.2f, 0), Bound), EMythicTriggerFacing::Front);
        TestEqual(TEXT("just outside it is a flank"),
                  GA::ResolveFacing(North, FVector(0.6f, 0.8f, 0), Bound), EMythicTriggerFacing::Flank);

        // A wider arc swallows what was a flank; a narrower one gives it back.
        TestEqual(TEXT("a wide arc counts a shoulder as front"),
                  GA::ResolveFacing(North, FVector(0.6f, 0.8f, 0), 0.5f), EMythicTriggerFacing::Front);
        TestEqual(TEXT("a narrow arc does not"),
                  GA::ResolveFacing(North, FVector(0.9f, 0.4f, 0), 0.99f), EMythicTriggerFacing::Flank);

        // Degenerate input has no arc, and must not satisfy a clause that asked for one.
        TestEqual(TEXT("actors on the same spot have no arc"),
                  GA::ResolveFacing(North, FVector::ZeroVector, Bound), EMythicTriggerFacing::Any);
        TestEqual(TEXT("an actor with no orientation has no arc"),
                  GA::ResolveFacing(FVector::ZeroVector, North, Bound), EMythicTriggerFacing::Any);

        TestTrue(TEXT("a clause asking for nothing accepts any arc"),
                 GA::PassesFacing(EMythicTriggerFacing::Any, EMythicTriggerFacing::Flank));
        TestTrue(TEXT("a behind clause accepts behind"),
                 GA::PassesFacing(EMythicTriggerFacing::Behind, EMythicTriggerFacing::Behind));
        TestFalse(TEXT("a behind clause rejects front"),
                  GA::PassesFacing(EMythicTriggerFacing::Behind, EMythicTriggerFacing::Front));
        TestFalse(TEXT("a behind clause rejects an unknown arc rather than passing by accident"),
                  GA::PassesFacing(EMythicTriggerFacing::Behind, EMythicTriggerFacing::Any));
    }

    // Sweep caps. A talent that hits everything nearby still needs a bound a designer can set.
    {
        auto Make = [](int32 Count) {
            TArray<AActor *> Out;
            for (int32 i = 1; i <= Count; ++i) {
                Out.Add(reinterpret_cast<AActor *>(static_cast<UPTRINT>(i)));
            }
            return Out;
        };

        TArray<AActor *> Five = Make(5);
        GA::LimitTargets(Five, 0);
        TestEqual(TEXT("no cap keeps every target"), Five.Num(), 5);

        TArray<AActor *> Capped = Make(5);
        GA::LimitTargets(Capped, 3);
        TestEqual(TEXT("a cap trims the sweep"), Capped.Num(), 3);

        TArray<AActor *> Under = Make(2);
        GA::LimitTargets(Under, 3);
        TestEqual(TEXT("a cap above the count changes nothing"), Under.Num(), 2);

        // The sweep is sorted nearest-first before the cap, so trimming keeps the closest.
        TArray<AActor *> Order = Make(4);
        GA::LimitTargets(Order, 2);
        TestEqual(TEXT("the cap keeps the front of the list"), Order[0], reinterpret_cast<AActor *>(static_cast<UPTRINT>(1)));
        TestEqual(TEXT("and the one after it"), Order[1], reinterpret_cast<AActor *>(static_cast<UPTRINT>(2)));

        TArray<AActor *> Empty;
        GA::LimitTargets(Empty, 3);
        TestEqual(TEXT("an empty sweep stays empty"), Empty.Num(), 0);
    }

    // Resistance. A talent must not ignore a resistance that a weapon proc respects.
    {
        TestEqual(TEXT("no resistance lets everything through"), GA::SurviveChanceFromResistance(0.0f), 1.0f);
        TestEqual(TEXT("half resistance halves the chance"), GA::SurviveChanceFromResistance(0.5f), 0.5f);
        TestEqual(TEXT("full resistance is immunity"), GA::SurviveChanceFromResistance(1.0f), 0.0f);

        // Resistances are rolled fractions and stack, so one above 1 is reachable exactly as dodge was.
        TestEqual(TEXT("resistance beyond full is still immunity, not negative"),
                  GA::SurviveChanceFromResistance(3.0f), 0.0f);
        TestFalse(TEXT("a fully resistant target is never affected"),
                  MythicCombat::RollSucceeds(GA::SurviveChanceFromResistance(1.0f), 0.0f));
        TestTrue(TEXT("an unresistant target is always affected"),
                 MythicCombat::RollSucceeds(GA::SurviveChanceFromResistance(0.0f), 0.999999f));
    }

    // A clause must do something. One with neither a status nor an effect is authored but inert.
    {
        FMythicTriggerSpec Empty;
        TestFalse(TEXT("a clause with no status and no effect is inert"), GA::HasPayload(Empty));

        FMythicTriggerSpec WithStatus;
        WithStatus.StatusToApply = FGameplayTag::RequestGameplayTag(FName("Status.Type.Burn"), false);
        TestTrue(TEXT("a status alone is a payload"), GA::HasPayload(WithStatus));

        FMythicTriggerSpec WithEffect;
        WithEffect.EffectToApply = UGameplayEffect::StaticClass();
        TestTrue(TEXT("an effect alone is a payload"), GA::HasPayload(WithEffect));
    }

    // The class must stay passive: talent validation rejects anything else, so a wrong default would make every
    // proc silently un-grantable.
    {
        const UMythicGA_Triggered *CDO = GetDefault<UMythicGA_Triggered>();
        TestEqual(TEXT("procs activate on spawn"), CDO->GetActivationPolicy(), EMythicAbilityActivationPolicy::OnSpawn);
    }

    return true;
}
