#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAbilityTargetIntentTest,"Mythic.Combat.Abilities.TargetIntent",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAbilityTargetIntentTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    // Use real player life/ASC/identity with the supported PvP hostility policy in this isolated world.
    TGuardValue<bool> FriendlyFire(GetMutableDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled,true);
    auto MakePlayer=[&]() {
        auto* Player=World->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
        return InitializeAbilityTestAvatar(Player);
    };
    auto* Source=MakePlayer(); auto* Near=MakePlayer(); auto* Far=MakePlayer();
    if (!TestTrue(TEXT("three production avatars initialize"),Source && Near && Far)) return false;
    Source->SetActorLocation(FVector(0,0,200));
    FVector Eye; FRotator EyeRotation; Source->GetActorEyesViewPoint(Eye,EyeRotation);
    Near->SetActorLocation(Eye+FVector(500,45,0)); Far->SetActorLocation(Eye+FVector(800,0,0));
    for (auto* Pawn:{Near,Far}) {
        Pawn->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Pawn->GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
        Pawn->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Pawn->GetCapsuleComponent()->SetCapsuleRadius(34);
    }
    Source->GetController()->SetControlRotation(FRotator::ZeroRotator);
    auto* Player=Source->GetPlayerState<AMythicPlayerState>();
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Skills=Player->GetSkillComponent();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.Winterbite")));
    Definition->ReusePolicy=EMythicAbilityReusePolicy::NoTimedCooldown;
    RegisterAbilityTestDefinition(Skills,Definition,true,false); Skills->ServerEquipSkill(0,Definition);
    auto* Spec=FindAbilityTestSpec(ASC,Definition);
    if (!TestNotNull(TEXT("real equipped spec exists"),Spec)) return false;
    auto* Window=Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance());
    if (!TestNotNull(TEXT("production window exists"),Window)) return false;
    Window->InitialIntent=EMythicAbilityIntent::Hostile; Window->IntentRange=1200;
    ASC->TryActivateAbility(Spec->Handle);
    TestTrue(TEXT("far on-ray target commits"),Window->IsWindowAuthority());
    TestTrue(TEXT("far pointed-at enemy wins over nearer enemy in the old broad cone"),Window->GetWindowTarget()==Far);
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);

    Near->SetActorLocation(Eye+FVector(350,160,0));

    auto* Wall=World->SpawnActor<AActor>(); auto* Block=NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Block); Wall->AddInstanceComponent(Block);
    Block->SetBoxExtent(FVector(20,1000,500)); Block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Block->SetCollisionObjectType(ECC_WorldStatic); Block->SetCollisionResponseToAllChannels(ECR_Ignore);
    Block->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); Block->RegisterComponent();
    Wall->SetActorLocation(FVector(400,0,200));
    const float Stamina=ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute());
    const int32 Uses=Skills->GetSkillUses(Definition);
    ASC->TryActivateAbility(Spec->Handle);
    TestFalse(TEXT("wall blocks both exact ray and assisted target"),Window->IsWindowAuthority());
    TestEqual(TEXT("wall rejection pays no stamina"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),Stamina);
    TestEqual(TEXT("wall rejection grants no practice"),Skills->GetSkillUses(Definition),Uses);
    Wall->Destroy();

    // Just outside the collision capsule but within the small authored assist cone.
    Far->SetActorLocation(Eye+FVector(800,70,0));
    ASC->TryActivateAbility(Spec->Handle);
    TestTrue(TEXT("small angular assistance admits a visible target"),Window->IsWindowAuthority() && Window->GetWindowTarget()==Far);
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    Far->SetActorLocation(Eye+FVector(800,300,0));
    ASC->TryActivateAbility(Spec->Handle);
    TestFalse(TEXT("broad off-reticle cone no longer silently acquires a target"),Window->IsWindowAuthority());
    return true;
}
#endif
