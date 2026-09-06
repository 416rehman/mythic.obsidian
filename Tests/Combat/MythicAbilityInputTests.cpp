#include "MythicAbilityInputTestAbility.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Input/MythicTags_Input.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

UMythicAbilityInputTestAbility::UMythicAbilityInputTestAbility() {
    InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::ServerOnly;
    NetSecurityPolicy=EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
    ActivationPolicy=EMythicAbilityActivationPolicy::OnInputTriggered;
    bCancelOnInputInterrupted=true;
}
void UMythicAbilityInputTestAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) {
    Trace.Add(TEXT("Activate"));
    if (bCancelDuringActivation) CastChecked<UMythicAbilitySystemComponent>(Info->AbilitySystemComponent.Get())->AbilityInputSpecCanceled(Handle);
    if (bClearAllDuringActivation) CastChecked<UMythicAbilitySystemComponent>(Info->AbilitySystemComponent.Get())->ClearAbilityInput();
}
bool UMythicAbilityInputTestAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayTagContainer* SourceTags,const FGameplayTagContainer* TargetTags,FGameplayTagContainer* RelevantTags) const {
    const auto* Spec=Info && Info->AbilitySystemComponent.IsValid() ? Info->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
    const auto* Instance=Spec ? Cast<UMythicAbilityInputTestAbility>(Spec->GetPrimaryInstance()) : nullptr;
    return Instance && Instance->bAllowActivation && Super::CanActivateAbility(Handle,Info,SourceTags,TargetTags,RelevantTags);
}
void UMythicAbilityInputTestAbility::InputPressed(FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo*,FGameplayAbilityActivationInfo) { Trace.Add(TEXT("Press")); }
void UMythicAbilityInputTestAbility::InputReleased(FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo*,FGameplayAbilityActivationInfo) { Trace.Add(TEXT("Release")); }
void UMythicAbilityInputTestAbility::CancelAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,bool bReplicate) {
    Trace.Add(TEXT("Cancel")); Super::CancelAbility(Handle,Info,Activation,bReplicate);
}

#if WITH_DEV_AUTOMATION_TESTS
namespace {
struct FInputFixture {
    UGameInstance* GI;
    UMythicAbilitySystemComponent* ASC;
    FInputFixture() {
        GI=NewObject<UGameInstance>(GEngine); GI->InitializeStandalone();
        auto* Actor=GI->GetWorld()->SpawnActor<AActor>();
        ASC=NewObject<UMythicAbilitySystemComponent>(Actor); Actor->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Actor,Actor);
    }
    ~FInputFixture() { DestroyOwnedAbilityTestWorld(GI); }
    FGameplayAbilitySpecHandle Give(FGameplayTag Tag={}) {
        FGameplayAbilitySpec Spec(UMythicAbilityInputTestAbility::StaticClass(),1); if (Tag.IsValid()) Spec.GetDynamicSpecSourceTags().AddTag(Tag);
        return ASC->GiveAbility(Spec);
    }
    UMythicAbilityInputTestAbility* Instance(FGameplayAbilitySpecHandle Handle) { return CastChecked<UMythicAbilityInputTestAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance()); }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityInputOrderTest,"Mythic.Combat.AbilityInput.OrderAndPhysicalEdges",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityInputOrderTest::RunTest(const FString&) {
    FInputFixture F; const auto H=F.Give(); auto* A=F.Instance(H);
    F.ASC->AbilityInputSpecPressed(H);
    for (int32 i=0;i<120;++i) F.ASC->AbilityInputSpecPressed(H);
    F.ASC->AbilityInputSpecReleased(H); F.ASC->AbilityInputSpecPressed(H);
    F.ASC->ProcessAbilityInput(0.016f,false);
    TestEqual(TEXT("three ordered callbacks, repeated physical down does not press again"),A->Trace.Num(),3);
    if (A->Trace.Num()==3) {
        TestEqual(TEXT("first press activates"),A->Trace[0],FName("Activate"));
        TestEqual(TEXT("release retains its position"),A->Trace[1],FName("Release"));
        TestEqual(TEXT("second press is a recast input"),A->Trace[2],FName("Press"));
    }
    F.ASC->SetAbilityInputSuppressed(true);
    TestEqual(TEXT("menu cancels once without an extra release"),A->Trace.Num(),4);
    TestFalse(TEXT("hold stopped"),A->IsActive());
    TestFalse(TEXT("spec is neutral"),F.ASC->FindAbilitySpecFromHandle(H)->InputPressed);
    F.ASC->SetAbilityInputSuppressed(true);
    TestEqual(TEXT("repeated suppression is idempotent"),A->Trace.Num(),4);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityInputReentrantCancelTest,"Mythic.Combat.AbilityInput.ReentrantCancel",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityInputReentrantCancelTest::RunTest(const FString&) {
    FInputFixture F; const auto H=F.Give(); auto* A=F.Instance(H); A->bCancelDuringActivation=true;
    F.ASC->AbilityInputSpecPressed(H); F.ASC->AbilityInputSpecReleased(H); F.ASC->AbilityInputSpecPressed(H);
    F.ASC->ProcessAbilityInput(0.016f,false);
    TestEqual(TEXT("cancel invalidates remaining copied edges"),A->Trace.Num(),2);
    TestFalse(TEXT("no release-only payload"),A->Trace.Contains(TEXT("Release")));
    TestFalse(TEXT("no canceled recast"),A->IsActive());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityInputSwapTest,"Mythic.Combat.AbilityInput.ReleaseOwnsOriginalSpec",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityInputSwapTest::RunTest(const FString&) {
    FInputFixture F; const auto Tag=INPUT_ACTION_SKILL_Q.GetTag();
    if (!TestTrue(TEXT("input tag exists"),Tag.IsValid())) return false;
    const auto Old=F.Give(Tag); F.ASC->AbilityInputTagPressed(Tag); F.ASC->ProcessAbilityInput(0.016f,false);
    F.ASC->ClearAbility(Old);
    const auto New=F.Give(Tag); auto* A=F.Instance(New);
    F.ASC->AbilityInputTagReleased(Tag); F.ASC->ProcessAbilityInput(0.016f,false);
    TestEqual(TEXT("replacement receives no old release"),A->Trace.Num(),0);
    F.ASC->AbilityInputTagPressed(Tag); F.ASC->ProcessAbilityInput(0.016f,false);
    TestEqual(TEXT("fresh press can use replacement"),A->Trace.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityHeldRetryCancelTest,"Mythic.Combat.AbilityInput.HeldRetryCancellation",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityHeldRetryCancelTest::RunTest(const FString&) {
    FInputFixture F; const auto HA=F.Give(),HB=F.Give(); auto* A=F.Instance(HA); auto* B=F.Instance(HB);
    // CDO policy is restored before leaving; the fixture worlds and specs never escape this test.
    auto* CDO=GetMutableDefault<UMythicAbilityInputTestAbility>(); const auto Old=CDO->ActivationPolicy;
    CDO->ActivationPolicy=EMythicAbilityActivationPolicy::WhileInputActive;
    A->bAllowActivation=B->bAllowActivation=false;
    F.ASC->AbilityInputSpecPressed(HA); F.ASC->AbilityInputSpecPressed(HB); F.ASC->ProcessAbilityInput(.016f,false);
    A->bAllowActivation=B->bAllowActivation=true; A->bClearAllDuringActivation=true;
    F.ASC->ProcessAbilityInput(.016f,false);
    CDO->ActivationPolicy=Old;
    TestEqual(TEXT("first retry activated then canceled"),A->Trace.Num(),2);
    TestEqual(TEXT("clear invalidates second copied held retry"),B->Trace.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityReleasedPreviewCancelTest,"Mythic.Combat.AbilityInput.ReleasedPreviewCancellation",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityReleasedPreviewCancelTest::RunTest(const FString&) {
    FInputFixture F; const auto H=F.Give(); auto* A=F.Instance(H);
    F.ASC->AbilityInputSpecPressed(H); F.ASC->AbilityInputSpecReleased(H); F.ASC->ProcessAbilityInput(.016f,false);
    TestTrue(TEXT("preview continues after a genuine release"),A->IsActive());
    F.ASC->ClearAbilityInput();
    TestFalse(TEXT("focus flush cancels continuing input-owned work"),A->IsActive());
    TestEqual(TEXT("flush adds cancellation, no synthetic release"),A->Trace.Num(),3);
    return true;
}
#endif
