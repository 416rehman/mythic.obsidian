#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Subsystem/SaveSystem/Character/SavedProficiency.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace {
class FMatureAbilityProviderRestore final : public IAutomationLatentCommand {
    FAutomationTestBase* Test;
    TStrongObjectPtr<UGameInstance> Game;
    double Deadline=FPlatformTime::Seconds()+20;
public:
    explicit FMatureAbilityProviderRestore(FAutomationTestBase* InTest,UGameInstance* InGame):Test(InTest),Game(InGame) {}
    virtual bool Update() override {
        auto* Registry=Game->GetSubsystem<UMythicItemizationDataRegistrySubsystem>();
        auto* Catalog=Game->GetSubsystem<UMythicAbilityCatalog>();
        if (!Registry->IsCoreSemanticReady() || !Catalog->IsReady()) {
            if (FPlatformTime::Seconds()<Deadline) return false;
            Test->AddError(TEXT("Configured semantic data and ability catalogue did not become ready within 20 seconds"));
            DestroyOwnedAbilityTestWorld(Game.Get()); return true;
        }
        auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player,false);
        auto* PC=Game->GetWorld()->SpawnActor<AMythicPlayerController>(); PC->PlayerState=Player; Player->SetOwner(PC);
        auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
        auto* Component=PC->GetProficiencyComponent();
        auto* Combat=LoadObject<UProficiencyDefinition>(nullptr,TEXT("/Game/Mythic/Proficiencies/Combat_Proficiency.Combat_Proficiency"));
        if (!Test->TestNotNull(TEXT("published Combat provider loads"),Combat)) { DestroyOwnedAbilityTestWorld(Game.Get()); return true; }
        FProficiency Track; Track.Definition=Combat; Component->Proficiencies={Track};
        FSerializedProficiencyData Saved; Saved.ProficiencyDefinition=Combat;
        Saved.CurrentXP=UProficiencyDefinition::CalcCumulativeXPForLevel(15,Combat)+0.5f;
        Test->TestTrue(TEXT("mature fixture starts with no new skill progress"),Skills->GetSkillProgress().IsEmpty());
        if (!Test->TestTrue(TEXT("legacy proficiency payload restores through the typed save helper"),FSerializedProficiencyHelper::Deserialize(Component,{Saved}))) {
            DestroyOwnedAbilityTestWorld(Game.Get()); return true;
        }
        Component->ApplyLoadedProficiencies();
        int32 Level=0;
        Test->TestTrue(TEXT("restored Combat XP resolves to level 15"),Component->TryGetLevelForDefinition(Combat,Level) && Level==15);
        for (const TCHAR* Identity:{TEXT("CinderLash"),TEXT("Winterbite"),TEXT("MortalMolt")}) {
            auto* Definition=LoadObject<UMythicSkillDefinition>(nullptr,*FString::Printf(TEXT("/Game/Mythic/Progression/Abilities/DA_Ability_%s.DA_Ability_%s"),Identity,Identity));
            Test->TestTrue(*FString::Printf(TEXT("mature milestone replay recovers %s without a prior progress row"),Identity),Definition && Skills->IsSkillUnlocked(Definition));
        }
        Test->TestEqual(TEXT("milestone replay creates exactly three learned rows"),Skills->GetSkillProgress().Num(),3);
        Test->TestTrue(TEXT("learning does not auto-equip or activate abilities"),ASC->GetActivatableAbilities().IsEmpty());
        TArray<FGameplayAttribute> Attributes; ASC->GetAllAttributes(Attributes);
        TMap<FGameplayAttribute,float> Before;
        for (const auto& Attribute:Attributes) Before.Add(Attribute,ASC->GetNumericAttributeBase(Attribute));
        Component->ApplyLoadedProficiencies();
        Test->TestEqual(TEXT("repeat mature replay cannot duplicate learned rows"),Skills->GetSkillProgress().Num(),3);
        for (const auto& Row:Before) Test->TestTrue(*FString::Printf(TEXT("repeat replay preserves permanent base %s"),*Row.Key.GetName()),FMath::IsNearlyEqual(Row.Value,ASC->GetNumericAttributeBase(Row.Key)));
        Test->TestTrue(TEXT("repeat replay cannot grant selected ability specs"),ASC->GetActivatableAbilities().IsEmpty());
        DestroyOwnedAbilityTestWorld(Game.Get()); return true;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicMatureAbilityProviderTest,"Mythic.Progression.Skills.MatureProficiencyProviderRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FMythicMatureAbilityProviderTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    Game->GetSubsystem<UMythicItemizationDataRegistrySubsystem>()->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda([](bool) {}));
    ADD_LATENT_AUTOMATION_COMMAND(FMatureAbilityProviderRestore(this,Game));
    return true;
}
#endif
