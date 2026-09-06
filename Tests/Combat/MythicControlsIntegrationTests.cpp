#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Input/MythicTags_Input.h"
#include "Itemization/Inventory/Fragments/Actionable/ConsumableEffectFragment.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicPlayerController.h"
#include "Misc/ScopeExit.h"
#include "Input/MythicTraversalComponent.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicFourSkillGrantRestore,"Mythic.Controls.FourSlotGrantAndRestore",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicFourSkillGrantRestore::RunTest(const FString&) {
    auto* Instance=NewObject<UGameInstance>(GEngine); Instance->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Instance); };
    auto* Player=Instance->GetWorld()->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("real authority skill owner"),Player)) return false;
    InitializeAbilityTestPlayer(Player);
    auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    const FGameplayTag Keys[]={INPUT_ACTION_SKILL_Q,INPUT_ACTION_SKILL_E,INPUT_ACTION_SKILL_R,INPUT_ACTION_SKILL_T};
    TArray<TSoftObjectPtr<UMythicSkillDefinition>> Saved;
    for (int32 Index=0;Index<4;++Index) {
        auto* Definition=NewObject<UMythicSkillDefinition>(Instance);
        Definition->Ability=UGameplayAbility::StaticClass(); Definition->Acquisition=EMythicAbilityAcquisition::RewardOnly;
        RegisterAbilityTestDefinition(Skills,Definition,false);
        Skills->ServerEquipSkill(Index,Definition);
        TestNull(TEXT("an available socket does not grant reward ownership"),Skills->GetSkillInSlot(Index));
        TestTrue(TEXT("authority reward learns definition"),Skills->LearnSkill(Definition));
        Skills->ServerEquipSkill(Index,Definition); Saved.Add(Definition);
        TestEqual(TEXT("socket has its canonical QERT identity"),Skills->GetSlotInputTag(Index),Keys[Index]);
    }
    const auto Verify=[&]() {
        for (int32 Index=0;Index<4;++Index) {
            TestTrue(TEXT("exact slot position retained"),Skills->GetSkillInSlot(Index)==Saved[Index].Get());
            int32 Count=0;
            for (const auto& Spec:ASC->GetActivatableAbilities()) if (!Spec.PendingRemove && Spec.SourceObject.Get()==Saved[Index].Get()) {
                ++Count;
                for (int32 Other=0;Other<4;++Other) TestEqual(TEXT("granted spec owns exactly its socket key"),Spec.GetDynamicSpecSourceTags().HasTagExact(Keys[Other]),Index==Other);
            }
            TestEqual(TEXT("exactly one granted spec per equipped definition"),Count,1);
        }
    };
    Verify();
    Skills->ServerEquipSkill(2,Saved[0].Get());
    TestTrue(TEXT("duplicate Q into R cannot displace R"),Skills->GetSkillInSlot(2)==Saved[2].Get());
    Skills->RestoreSkills(Saved,4); Skills->SetRestoring(false); Verify();
    Skills->RestoreSkills(Saved,4); Skills->SetRestoring(false); Verify();
    Skills->GrantSlot(); TestEqual(TEXT("legacy reward cannot grow a fifth slot"),Skills->GetUnlockedSlots(),4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicConsumableCompleteSaveRoundTrip,"Mythic.Controls.ConsumableCompleteSaveRoundTrip",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicConsumableCompleteSaveRoundTrip::RunTest(const FString&) {
    auto* Instance=NewObject<UGameInstance>(GEngine); Instance->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Instance); };
    auto* Owner=Instance->GetWorld()->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("authority item owner"),Owner)) return false;
    auto* Definition=NewObject<UItemDefinition>(Instance); Definition->StackSizeMax=20;
    auto* Authored=NewObject<UConsumableEffectFragment>(Definition); Definition->Fragments.Add(Authored);
    Authored->InputTag=FGameplayTag();
    auto& Config=Authored->ConsumableEffectConfig; Config.bHealing=true; Config.bUtility=true; Config.QuickUseVariant=TEXT("PreservedVariant");
    for (int32 Index=0;Index<3;++Index) {
        auto& Entry=Config.Effects.AddDefaulted_GetRef(); Entry.GameplayEffectClass=UGameplayEffect::StaticClass(); Entry.EffectLevel=2.f+Index;
        Entry.SetByCallerMagnitudes.Add(INPUT_ACTION_SKILL_Q,22.f+Index);
    }
    // An existing registered tag exercises exact payload serialization independently of effect execution.
    auto* Item=NewObject<UMythicItemInstance>(Owner); Item->SetOwner(Owner); Item->InitializeFixtureForTests(Definition,7,1);
    TArray<uint8> Bytes;
    { FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Ar(Writer,false); Ar.ArIsSaveGame=true; Item->Serialize(Ar); TestFalse(TEXT("complete item saves"),Ar.IsError()); }
    const auto Load=[&]() {
        auto* Restored=NewObject<UMythicItemInstance>(Owner); Restored->SetOwner(Owner);
        FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Ar(Reader,true); Ar.ArIsSaveGame=true; Restored->Serialize(Ar);
        TestFalse(TEXT("complete consumable loads"),Ar.IsError()); return Restored;
    };
    auto* Restored=Load();
    TestEqual(TEXT("quantity remains exact"),Restored->GetStacks(),7);
    TestEqual(TEXT("physical identity survives save"),Restored->GetItemInstanceGuid(),Item->GetItemInstanceGuid());
    const auto* Fragment=Restored->GetFragment<UConsumableEffectFragment>();
    if (!TestNotNull(TEXT("typed effect fragment restored"),Fragment)) return false;
    TestTrue(TEXT("GE classes, levels, payload and quick-use classification survive array reconstruction"),Fragment->CanBeStackedWith(Authored));
    TestEqual(TEXT("no effect entries disappear"),Fragment->ConsumableEffectConfig.Effects.Num(),3);
    TestTrue(TEXT("restored fragment belongs to exact restored item"),Fragment->GetOwningItemInstance()==Restored);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicRestoreCancelsOwnedMantle,
    "Mythic.Controls.RestoreCancelsOwnedMantle",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicRestoreCancelsOwnedMantle::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    auto* State=World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("real authority state"),State)) return false;
    InitializeAbilityTestPlayer(State);
    auto* Avatar=InitializeAbilityTestAvatar(State);
    if (!TestNotNull(TEXT("production initialized avatar"),Avatar)) return false;
    auto* Traversal=Avatar->FindComponentByClass<UMythicTraversalComponent>();
    auto* Move=Avatar->GetCharacterMovement();
    if (!TestNotNull(TEXT("native traversal owner"),Traversal) || !Move) return false;
    const auto Box=[&](FVector Center,FVector Extent) {
        auto* Actor=World->SpawnActor<AActor>();
        if (!Actor) return static_cast<UBoxComponent*>(nullptr);
        auto* Shape=NewObject<UBoxComponent>(Actor); Actor->SetRootComponent(Shape);
        Shape->InitBoxExtent(Extent); Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Shape->SetCollisionObjectType(ECC_WorldStatic); Shape->SetCollisionResponseToAllChannels(ECR_Block);
        Shape->RegisterComponent(); Actor->SetActorLocation(Center); return Shape;
    };
    if (!TestNotNull(TEXT("real floor"),Box(FVector(0,0,-10),FVector(600,500,10)))
        || !TestNotNull(TEXT("real 80cm ledge"),Box(FVector(160,0,40),FVector(80,140,40)))) return false;
    Avatar->SetActorLocation(FVector(-300,0,98.5),false,nullptr,ETeleportType::TeleportPhysics);
    Move->SetMovementMode(MOVE_Walking);
    FSerializedCharacterData Saved;
    if (!TestTrue(TEXT("typed production snapshot"),FSerializedCharacterData::Serialize(State,Saved))) return false;
    const auto Entity=State->GetPersistentEntityId();
    Avatar->SetActorLocation(FVector(0,0,98.5),false,nullptr,ETeleportType::TeleportPhysics);
    FMythicMantleRoute Route;
    if (!TestTrue(TEXT("production full capsule route admits the real ledge"),
        UMythicTraversalComponent::BuildMantleRoute(Avatar,FVector::ForwardVector,Route))) return false;
    const FVector OldEndpoint=Route.Points.Last();
    // Call the real direction-only authority entry point. Do not fabricate
    // ActiveRequest, root-motion IDs, geometry acceptance or montage state.
    auto* Request=Traversal->FindFunction(TEXT("ServerRequestMantle"));
    if (!TestNotNull(TEXT("native authority request"),Request)) return false;
    FStructOnScope Params(Request);
    auto* RequestId=FindFProperty<FUInt32Property>(Request,TEXT("Request"));
    auto* Direction=FindFProperty<FStructProperty>(Request,TEXT("Direction"));
    if (!TestNotNull(TEXT("typed request serial"),RequestId) || !TestNotNull(TEXT("typed direction"),Direction)) return false;
    RequestId->SetPropertyValue_InContainer(Params.GetStructMemory(),1);
    *Direction->ContainerPtrToValuePtr<FVector_NetQuantizeNormal>(Params.GetStructMemory())=FVector_NetQuantizeNormal(FVector::ForwardVector);
    Traversal->ProcessEvent(Request,Params.GetStructMemory());
    if (!TestTrue(TEXT("real authority mantle is active"),Traversal->IsMantling())) return false;
    auto MantleSource=Move->GetRootMotionSource(TEXT("SystemMantle_1_0"));
    if (!TestTrue(TEXT("production CMC source was installed"),MantleSource.IsValid())) return false;
    TestFalse(TEXT("admitted source starts live"),MantleSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));
    Traversal->ReleaseJump();
    TestTrue(TEXT("ordinary jump release does not finish admitted mantle"),Traversal->IsMantling());
    auto Sibling=MakeShared<FRootMotionSource_ConstantForce>();
    Sibling->InstanceName=TEXT("RestoreTestSibling"); Sibling->Duration=5;
    Sibling->AccumulateMode=ERootMotionAccumulateMode::Additive;
    Sibling->Force=FVector(0,10,0);
    const uint16 SiblingId=Move->ApplyRootMotionSource(Sibling);
    if (!TestTrue(TEXT("unrelated native source is installed"),SiblingId!=0)) return false;
    if (!TestTrue(TEXT("same-pawn production restore succeeds"),FSerializedCharacterData::Deserialize(State,Saved))) return false;
    TestFalse(TEXT("restore clears admitted mantle owner"),Traversal->IsMantling());
    // UE marks a source for removal; actual CMC cleanup happens on movement.
    TestTrue(TEXT("only the old mantle source is retired"),MantleSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));
    TestTrue(TEXT("sibling source retains exact identity"),Move->GetRootMotionSourceByID(SiblingId)==Sibling);
    TestFalse(TEXT("sibling source is not canceled"),Sibling->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));
    TestTrue(TEXT("saved transform applies after cancellation"),Avatar->GetActorTransform().Equals(Saved.SavedTransform,0.01f));
    TestTrue(TEXT("same character incarnation retained"),State->GetPersistentEntityId()==Entity);
    // A real CMC update may move under the sibling source/gravity, but cannot
    // resume the old 130cm-forward mantle route from the restored -300cm origin.
    const FVector Restored=Avatar->GetActorLocation();
    Move->TickComponent(1.f/60.f,LEVELTICK_All,&Move->PrimaryComponentTick);
    TestTrue(TEXT("CMC cannot pull restored pawn toward old mantle endpoint"),
        FMath::Abs(Avatar->GetActorLocation().X-Restored.X)<1.f
        && FVector::Dist2D(Avatar->GetActorLocation(),OldEndpoint)>300.f);
    TestFalse(TEXT("movement cleanup does not revive mantle"),Traversal->IsMantling());
    Move->RemoveRootMotionSourceByID(SiblingId);
    return true;
}
#endif
