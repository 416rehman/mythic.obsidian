#include "Misc/AutomationTest.h"
#include "GAS/Abilities/MythicGA_AbilityWindow.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "Settings/MythicDeveloperSettings.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace {
TArray<UMythicSkillDefinition*> ApprovedAbilities(FAutomationTestBase& Test) {
    TArray<UMythicSkillDefinition*> Definitions;
    TSet<FSoftObjectPath> Seen;
    for (const auto& Path:GetDefault<UMythicDeveloperSettings>()->AbilityCatalog) {
        Test.TestFalse(TEXT("catalogue identities are unique"),Seen.Contains(Path.ToSoftObjectPath())); Seen.Add(Path.ToSoftObjectPath());
        auto* Definition=Path.LoadSynchronous();
        if (Test.TestNotNull(TEXT("configured ability metadata loads"),Definition)) Definitions.Add(Definition);
    }
    return Definitions;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillContentTest,"Mythic.Progression.SkillContent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMythicSkillContentTest::RunTest(const FString&) {
    const auto Definitions=ApprovedAbilities(*this);
    TestEqual(TEXT("the accepted four abilities are configured"),Definitions.Num(),4);
    TSet<FGameplayTag> Cooldowns; TSet<UClass*> Implementations; TSet<FName> Names;
    for (const auto* Definition:Definitions) {
        TArray<FText> Errors; const bool Valid=Definition->ValidateDefinition(Errors);
        TestTrue(*Definition->GetName(),Valid); for (const auto& Error:Errors) AddError(Error.ToString());
        TestTrue(TEXT("only the new authored root is in the live catalogue"),Definition->GetPathName().StartsWith(TEXT("/Game/Mythic/Progression/Abilities/")));
        TestNull(TEXT("metadata has no legacy hard ability reference"),Definition->Ability.Get());
        auto* Runtime=Definition->RuntimeAbility.LoadSynchronous();
        if (!TestNotNull(TEXT("equippable runtime class loads"),Runtime)) continue;
        const auto* Window=Cast<UMythicGA_AbilityWindow>(Runtime->GetDefaultObject());
        if (!TestNotNull(TEXT("runtime uses the bounded GAS window"),Window)) continue;
        if (Definition->HasTimedCooldown()) {
            TestFalse(TEXT("timed ability cooldown keys cannot collide"),Cooldowns.Contains(Definition->CooldownTag)); Cooldowns.Add(Definition->CooldownTag);
        }
        TestFalse(TEXT("each new ability has independently authored behavior"),Implementations.Contains(Runtime)); Implementations.Add(Runtime);
        Names.Add(Definition->GetFName());
        TestNotNull(TEXT("icon asset loads"),Definition->Icon.LoadSynchronous());
        const UFunction* Sequence=Runtime->FindFunctionByName(TEXT("OnWindowStarted"));
        TestTrue(TEXT("the ability owns an executable Blueprint sequence"),Sequence && Sequence->GetOuterUClass()==Runtime && !Sequence->Script.IsEmpty());
        TestTrue(TEXT("window safety duration is finite and bounded"),FMath::IsFinite(Window->Duration) && Window->Duration>0 && Window->Duration<=120);
        TestTrue(TEXT("new abilities use prediction-capable GAS"),Window->GetNetExecutionPolicy()==EGameplayAbilityNetExecutionPolicy::LocalPredicted);
        for (const auto& Budget:Window->Budgets) TestTrue(TEXT("authored event budgets are bounded"),!Budget.Key.IsNone() && Budget.Value>=0 && Budget.Value<=256);
    }
    for (const TCHAR* Identity:{TEXT("CinderLash"),TEXT("ObsidianMaw"),TEXT("Winterbite"),TEXT("MortalMolt")})
        TestTrue(*FString::Printf(TEXT("accepted ability %s is configured"),Identity),Names.Contains(FName(*FString::Printf(TEXT("DA_Ability_%s"),Identity))));
    for (const TCHAR* Identity:{TEXT("BellOfLastStand"),TEXT("FuneralSun"),TEXT("Stormroad"),TEXT("BriarSovereign"),TEXT("BorrowedDawn"),TEXT("CrownOfDebts")})
        TestFalse(*FString::Printf(TEXT("retired ability %s is absent from the live catalogue"),Identity),Names.Contains(FName(*FString::Printf(TEXT("DA_Ability_%s"),Identity))));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillModifierContentTest,"Mythic.Progression.SkillModifierContent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMythicSkillModifierContentTest::RunTest(const FString&) {
    const auto Definitions=ApprovedAbilities(*this);
    for (const auto* Definition:Definitions) {
        TArray<FText> Errors;
        TestTrue(*Definition->GetName(),Definition->ValidateDefinition(Errors));
        for (const auto& Error:Errors) AddError(Error.ToString());
        // Modifier count and behavior are authored choices. Validate any supplied
        // modifiers without requiring filler transformations or shared formulas.
        TSet<FName> Identities;
        for (const auto& Modifier:Definition->Modifiers) {
            TestFalse(TEXT("stable modifier ID is present"),Modifier.ModifierId.IsNone());
            TestFalse(TEXT("modifier IDs do not collide"),Identities.Contains(Modifier.ModifierId)); Identities.Add(Modifier.ModifierId);
            TestFalse(TEXT("modifier name is readable"),Modifier.Name.IsEmpty()); TestFalse(TEXT("modifier describes its replacement behavior"),Modifier.Description.IsEmpty());
        }
    }
    return true;
}
#endif
