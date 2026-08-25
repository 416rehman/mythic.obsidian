#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AutomationTest.h"

#include "GAS/Abilities/MythicGA_Skill.h"
#include "Progression/Skills/MythicSkillDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
    constexpr int32 ExpectedSkills = 16;

    void LoadSkillDefinitions(TArray<UMythicSkillDefinition *> &Out) {
        FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        Module.Get().SearchAllAssets(true);

        TArray<FAssetData> Assets;
        Module.Get().GetAssetsByClass(UMythicSkillDefinition::StaticClass()->GetClassPathName(), Assets, true);
        for (const FAssetData &Asset : Assets) {
            if (UMythicSkillDefinition *Skill = Cast<UMythicSkillDefinition>(Asset.GetAsset())) {
                Out.Add(Skill);
            }
        }
        Out.Sort([](const UMythicSkillDefinition &A, const UMythicSkillDefinition &B) {
            return A.GetName() < B.GetName();
        });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillContentTest,
                                 "Mythic.Progression.SkillContent",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The sixteen skills are rows on UMythicGA_Skill, not sixteen graphs, so what makes each one a skill is its
 * authored data. A row that reaches nobody, moves nobody and changes nobody still loads, still shows its icon
 * and still fires - it just does nothing, which is the failure this guards.
 */
bool FMythicSkillContentTest::RunTest(const FString &Parameters) {
    TArray<UMythicSkillDefinition *> Skills;
    LoadSkillDefinitions(Skills);

    AddInfo(FString::Printf(TEXT("%d skill definitions found, %d expected"), Skills.Num(), ExpectedSkills));
    if (!TestEqual(TEXT("every authored skill is present"), Skills.Num(), ExpectedSkills)) {
        return false;
    }

    int32 Reaching = 0;
    int32 Moving = 0;
    int32 WithStatus = 0;

    for (const UMythicSkillDefinition *Skill : Skills) {
        const FString Id = Skill->GetName();

        TestFalse(*FString::Printf(TEXT("%s is named"), *Id), Skill->Name.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s is described"), *Id), Skill->Description.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s carries an icon"), *Id), Skill->Icon.IsNull());

        if (!TestTrue(*FString::Printf(TEXT("%s has an ability"), *Id), Skill->HasPayload())) {
            continue;
        }

        const UMythicGA_Skill *Ability = Cast<UMythicGA_Skill>(Skill->Ability->GetDefaultObject());
        if (!TestNotNull(*FString::Printf(TEXT("%s's ability is a UMythicGA_Skill"), *Id), Ability)) {
            continue;
        }

        const bool bReaches = Ability->Shape.Radius > 0.0f;
        const bool bMoves = Ability->Movement != EMythicSkillMovement::None && Ability->MovementDistance > 0.0f;
        const bool bChangesCaster = Ability->SelfEffect != nullptr;

        TestTrue(*FString::Printf(TEXT("%s does something: it reaches, moves or changes the caster"), *Id),
                 bReaches || bMoves || bChangesCaster);

        Reaching += bReaches ? 1 : 0;
        Moving += bMoves ? 1 : 0;

        if (bReaches) {
            // A skill that reaches has to be able to hurt what it reaches, or the reach is decoration.
            TestNotNull(*FString::Printf(TEXT("%s can calculate damage"), *Id),
                        Ability->Damage.DamageCalculationEffect.Get());
            TestNotNull(*FString::Printf(TEXT("%s can apply damage"), *Id),
                        Ability->Damage.DamageApplicationEffect.Get());
        }

        // An arc or cone with no width selects nothing; a full 360 is the sphere case and is legitimate.
        if (Ability->Shape.Shape == EMythicSkillShape::Arc || Ability->Shape.Shape == EMythicSkillShape::Cone) {
            TestTrue(*FString::Printf(TEXT("%s has a usable angle"), *Id),
                     Ability->Shape.AngleDegrees > 0.0f && Ability->Shape.AngleDegrees <= 360.0f);
        }

        if (Ability->StatusToApply.IsValid()) {
            ++WithStatus;
            // The registry keys on Status.Type.*; a GAS.Debuff.* tag here resolves to nothing and applies nothing.
            TestTrue(*FString::Printf(TEXT("%s names a Status.Type tag"), *Id),
                     Ability->StatusToApply.ToString().StartsWith(TEXT("Status.Type.")));
            TestTrue(*FString::Printf(TEXT("%s has a rollable status chance"), *Id),
                     Ability->StatusChance > 0.0f && Ability->StatusChance <= 1.0f);
        }
    }

    AddInfo(FString::Printf(TEXT("of %d skills: %d reach, %d move, %d carry a status"),
                            Skills.Num(), Reaching, Moving, WithStatus));

    // The sweep must find a spread, not sixteen copies of one row.
    TestTrue(TEXT("skills that reach exist"), Reaching > 0);
    TestTrue(TEXT("skills that move exist"), Moving > 0);
    TestTrue(TEXT("skills that carry a status exist"), WithStatus > 0);
    TestTrue(TEXT("not every skill reaches, so the defensive and movement ones are real"), Reaching < Skills.Num());

    return true;
}

#endif
