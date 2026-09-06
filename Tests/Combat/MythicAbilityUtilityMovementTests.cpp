#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicUtilityWindowTestProbe.h"
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Misc/ScopeExit.h"

namespace {
class FUtilityTravelCommand final : public IAutomationLatentCommand {
public:
    FUtilityTravelCommand(FAutomationTestBase* InTest,UGameInstance* InGame,AMythicCharacter_Player* InAvatar,
        UMythicUtilityWindowTestProbe* InWindow,UMythicAbilitySystemComponent* InASC,FGameplayAbilitySpecHandle InHandle,FVector InStart)
        : Test(InTest),Game(InGame),Avatar(InAvatar),Window(InWindow),ASC(InASC),Handle(InHandle),Start(InStart),Deadline(FPlatformTime::Seconds()+15) {}
    virtual ~FUtilityTravelCommand() override {
        Window->OnUtilityMovementResolved.Clear();
        ASC->OnAbilityMovementStarted.Remove(Started); ASC->OnAbilityMovementEnded.Remove(Ended);
        Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        DestroyOwnedAbilityTestWorld(Game.Get());
    }
    virtual bool Update() override {
        if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("utility travel fixture exceeded its15-second frame deadline")); return true; }
        if (LastEngineFrame==GFrameCounter) return false;
        LastEngineFrame=GFrameCounter;
        auto* World=Game->GetWorld(); auto* Move=Avatar->GetCharacterMovement();
        if (!bInitialized) {
            Window->OnUtilityMovementResolved.AddLambda([this](const auto& Receipt){ Receipts.Add(Receipt); });
            Started=ASC->OnAbilityMovementStarted.AddLambda([this](auto* Ability,const FVector&){ if (Ability==Window.Get()) ++Begins; });
            Ended=ASC->OnAbilityMovementEnded.AddLambda([this](auto* Ability,const FVector&){ if (Ability==Window.Get()) ++Ends; });
            ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),200);
            ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),200);
            bInitialized=true;
        }
        if (Frames==0) {
            Receipts.Reset(); Begins=Ends=0;
            Avatar->SetActorLocation(Start); Avatar->ConsumeMovementInputVector(); Avatar->SetActorRotation(FRotator::ZeroRotator);
            Move->StopMovementImmediately(); Move->SetMovementMode(MOVE_Walking);
            FGameplayTagContainer FailureTags,OwnerTags;
            const bool bCanActivate=Window->CanActivateAbility(Handle,ASC->AbilityActorInfo.Get(),nullptr,nullptr,&FailureTags);
            const bool bActivated=ASC->TryActivateAbility(Handle);
            if (!bActivated || !Window->IsWindowAuthority()) {
                ASC->GetOwnedGameplayTags(OwnerTags);
                const auto* Player=Cast<AMythicPlayerState>(ASC->GetOwnerActor());
                const auto* Skills=Player ? Player->GetSkillComponent() : nullptr;
                const auto* Spec=ASC->FindAbilitySpecFromHandle(Handle);
                const auto* Definition=Spec ? Cast<UMythicSkillDefinition>(Spec->SourceObject.Get()) : nullptr;
                TArray<FVector> Path;
                const FVector Direction=Window->GetMovementIntentDirection(true).GetSafeNormal2D();
                const bool bPath=Window->ValidateAbilityEscapePath(Direction,320,Path);
                Test->AddInfo(FString::Printf(TEXT("Utility activation scenario%d: can%d/try%d/active%d/window%d, ready%d/owns%d, stamina%.3f, mode%d, path%d/%d, preparation%s, failure[%s], owner[%s], ASCavatar%s/controller%s"),
                    Scenario,bCanActivate,bActivated,Window->IsActive(),Window->IsWindowAuthority(),Skills && Skills->IsReady(),
                    Skills && Skills->OwnsGrantedSpec(Definition,Handle),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),
                    static_cast<int32>(Move->MovementMode),bPath,Path.Num(),*Window->EscapePreparation.ToString(),*FailureTags.ToStringSimple(),*OwnerTags.ToStringSimple(),
                    *GetNameSafe(ASC->GetAvatarActor()),*GetNameSafe(Avatar->GetController())));
                auto* Capsule=Avatar->GetCapsuleComponent();
                auto* Presentation=Avatar->FindComponentByClass<UMythicEntityPresentationComponent>();
                float Radius=0,HalfHeight=0; Capsule->GetScaledCapsuleSize(Radius,HalfHeight);
                const float Step=FMath::Clamp(Move->MaxStepHeight,0.0f,100.0f);
                const FVector ActualStart=Avatar->GetActorLocation();
                FVector Candidate=ActualStart+Direction*40+FVector(0,0,Step);
                FFindFloorResult Floor; Move->ComputeFloorDist(Candidate,2*Step+6,2*Step+6,Floor,Radius);
                Candidate.Z-=Floor.bLineTrace ? Floor.LineDist : Floor.FloorDist; Candidate.Z+=2;
                FHitResult Corridor;
                FCollisionQueryParams Query(SCENE_QUERY_STAT(UtilityFixtureDiagnosis),false,Avatar.Get());
                FCollisionResponseParams Responses; Capsule->InitSweepCollisionParams(Query,Responses);
                const bool bCorridor=World->SweepSingleByChannel(Corridor,ActualStart,Candidate,Capsule->GetComponentQuat(),Capsule->GetCollisionObjectType(),
                    FCollisionShape::MakeCapsule(Radius,HalfHeight),Query,Responses);
                Test->AddInfo(FString::Printf(TEXT("Utility geometry: start%s/candidate%s, intent%s, capsule%.3f/%.3f/collision%d, updated%s/same%d, presentationValid%d, floor block%d/walk%d/penetration%d/line%d/dist%.3f/lineDist%.3f/actor%s/component%s, corridor%d/time%.6f/penetration%d/actor%s/component%s"),
                    *ActualStart.ToString(),*Candidate.ToString(),*Window->GetMovementIntentDirection(true).ToString(),Radius,HalfHeight,static_cast<uint8>(Capsule->GetCollisionEnabled()),
                    *GetNameSafe(Move->UpdatedComponent),Move->UpdatedComponent==Capsule,Presentation && Presentation->GetPresentationInstance().IsValid(),
                    Floor.bBlockingHit,Floor.IsWalkableFloor(),Floor.HitResult.bStartPenetrating,Floor.bLineTrace,Floor.FloorDist,Floor.LineDist,
                    *GetNameSafe(Floor.HitResult.GetActor()),*GetNameSafe(Floor.HitResult.GetComponent()),bCorridor,Corridor.Time,Corridor.bStartPenetrating,
                    *GetNameSafe(Corridor.GetActor()),*GetNameSafe(Corridor.GetComponent())));
            }
            if (!Test->TestTrue(TEXT("fresh activation is allowed after callbacks unwind"),bActivated && Window->IsWindowAuthority())) return true;
            const FGuid Action=Window->BeginAbilityAction(TEXT("Escape"));
            if (!Test->TestTrue(TEXT("real travel starts through owned preparation"),Window->StartPreparedUtilityEscape(Action,Window->EscapePreparation))) return true;
            if (Scenario==1) {
                Obstacle=World->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Obstacle.Get());
                Obstacle->SetRootComponent(Box); Obstacle->AddInstanceComponent(Box);
                Box->SetBoxExtent(FVector(10,300,100)); Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionResponseToAllChannels(ECR_Block);
                Box->RegisterComponent(); Obstacle->SetActorLocation(FVector(-170,0,100));
            }
        }
        // A standalone fixture world is not editor PIE. Tick it once per actual engine frame
        // so TickTaskManager and TimerManager both advance through their normal frame guards.
        World->Tick(LEVELTICK_All,1.0f/60.0f);
        if (++Frames<36) return false;
        Test->AddInfo(FString::Printf(TEXT("Utility travel scenario%d: location%s, receipts%d, begin%d/end%d, world%.3f, rootmotion%d"),
            Scenario,*Avatar->GetActorLocation().ToString(),Receipts.Num(),Begins,Ends,World->GetTimeSeconds(),Move->GetRootMotionSource(TEXT("UtilityEscape")).IsValid()));
        Test->TestEqual(TEXT("real motion begins exactly once"),Begins,1);
        Test->TestEqual(TEXT("real motion ends exactly once"),Ends,1);
        Test->TestEqual(TEXT("real timed motion publishes exactly one result"),Receipts.Num(),1);
        if (Receipts.Num()==1) {
            Test->TestEqual(TEXT("native movement distinguishes reached from late obstruction"),Receipts.Last().Outcome,
                Scenario==1 ? EMythicAbilityMovementOutcome::Blocked : EMythicAbilityMovementOutcome::Reached);
            Test->TestTrue(TEXT("receipt location comes from actual capsule"),Receipts.Last().ActualEnd.Equals(Avatar->GetActorLocation(),2));
            if (Scenario==1) Test->TestTrue(TEXT("late collision stops before the admitted endpoint"),FVector::Dist2D(Receipts.Last().ActualEnd,Receipts.Last().AdmittedEnd)>40);
        }
        Test->TestFalse(TEXT("finished utility removes its native root motion source"),Move->GetRootMotionSource(TEXT("UtilityEscape")).IsValid());
        Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        if (Obstacle.IsValid()) { Obstacle->Destroy(); Obstacle.Reset(); }
        Frames=0; return ++Scenario==2;
    }
private:
    FAutomationTestBase* Test;
    TStrongObjectPtr<UGameInstance> Game;
    TStrongObjectPtr<AMythicCharacter_Player> Avatar;
    TStrongObjectPtr<UMythicUtilityWindowTestProbe> Window;
    TStrongObjectPtr<UMythicAbilitySystemComponent> ASC;
    FGameplayAbilitySpecHandle Handle;
    FVector Start;
    TWeakObjectPtr<AActor> Obstacle;
    TArray<FMythicAbilityMovementReceipt> Receipts;
    FDelegateHandle Started,Ended;
    int32 Frames=0,Scenario=0,Begins=0,Ends=0;
    uint64 LastEngineFrame=MAX_uint64;
    double Deadline;
    bool bInitialized=false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityUtilityMovementTest,"Mythic.Combat.Abilities.UtilityMovement",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityUtilityMovementTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    bool bLatentOwnsWorld=false;
    ON_SCOPE_EXIT { if (!bLatentOwnsWorld) DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    const auto Box=[&](FVector Location,FVector Extent) {
        auto* Actor=World->SpawnActor<AActor>(); auto* Component=NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Component); Actor->AddInstanceComponent(Component);
        Component->SetBoxExtent(Extent); Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Component->SetCollisionObjectType(ECC_WorldStatic); Component->SetCollisionResponseToAllChannels(ECR_Block);
        Component->RegisterComponent(); Actor->SetActorLocation(Location); return Actor;
    };
    Box(FVector(0,0,-50),FVector(1500,1500,50));
    auto* Player=World->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Avatar=InitializeAbilityTestAvatar(Player);
    if (!TestNotNull(TEXT("real movable character exists"),Avatar)) return false;
    World->GetWorldSettings()->NotifyBeginPlay();
    if (!TestTrue(TEXT("isolated travel world actually begins its actor lifecycle"),World->HasBegunPlay() && Avatar->HasActorBegunPlay())) return false;
    auto* Capsule=Avatar->GetCapsuleComponent(); auto* Move=Avatar->GetCharacterMovement();
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Capsule->SetCollisionResponseToAllChannels(ECR_Block);
    const FVector Start(0,0,Capsule->GetScaledCapsuleHalfHeight()+2);
    Avatar->SetActorLocation(Start); Avatar->SetActorRotation(FRotator::ZeroRotator); Move->SetMovementMode(MOVE_Walking);
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Skills=Player->GetSkillComponent();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),200);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),200);
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    Definition->RuntimeAbility=UMythicUtilityWindowTestProbe::StaticClass(); Definition->ReusePolicy=EMythicAbilityReusePolicy::NoTimedCooldown;
    Definition->CooldownSeconds=0; Definition->CooldownTag={}; Definition->CooldownEffect=nullptr;
    FMythicAbilityActionProfile Utility; Utility.ActionId=TEXT("Escape"); Utility.Work=EMythicAbilityActionWork::Utility;
    Utility.bScaleWithPrimaryStats=false; Utility.MaximumTargets=1;
    Utility.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Target.Self")));
    Utility.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.Movement")));
    Utility.Parameters={{TEXT("Distance"),320,40,600},{TEXT("Transit"),0.22f,0.05f,1}}; Definition->Actions={Utility};
    RegisterAbilityTestDefinition(Skills,Definition,true,false); Skills->ServerEquipSkill(0,Definition);
    auto* Spec=FindAbilityTestSpec(ASC,Definition);
    auto* Window=Spec ? Cast<UMythicUtilityWindowTestProbe>(Spec->GetPrimaryInstance()):nullptr;
    if (!TestNotNull(TEXT("real equipped utility probe exists"),Window)) return false;
    TArray<FMythicAbilityMovementReceipt> Receipts;
    Window->OnUtilityMovementResolved.AddLambda([&](const auto& Receipt){ Receipts.Add(Receipt); });
    ON_SCOPE_EXIT { Window->OnUtilityMovementResolved.Clear(); };
    Avatar->AddMovementInput(FVector::RightVector,1);
    TestTrue(TEXT("current lateral command admits a grounded escape"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    TestTrue(TEXT("escape follows movement independently of camera aim"),Window->GetMovementIntentDirection(true).Equals(FVector::RightVector,0.001));
    const FGuid Action=Window->BeginAbilityAction(TEXT("Escape")),Preparation=Window->EscapePreparation;
    TestTrue(TEXT("precommit capsule path has an authority token"),Preparation.IsValid());
    TestTrue(TEXT("prepared escape starts one native movement task"),Window->StartPreparedUtilityEscape(Action,Preparation));
    TestFalse(TEXT("same action and prepared endpoint cannot start twice"),Window->StartPreparedUtilityEscape(Action,Preparation));
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    TestTrue(TEXT("cancellation emits exactly one owned movement receipt"),Receipts.Num()==1 && Receipts[0].Outcome==EMythicAbilityMovementOutcome::Cancelled
        && Receipts[0].ActionGroupId==Action && Receipts[0].AdmittedEnd.Y>300);
    TestTrue(TEXT("cancelled receipt records actual location"),Receipts.Num()==1 && Receipts[0].ActualEnd.Equals(Avatar->GetActorLocation(),0.001));
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    TestEqual(TEXT("repeated cancellation cannot publish again"),Receipts.Num(),1);
    Avatar->ConsumeMovementInputVector(); Avatar->SetActorLocation(Start); Move->SetMovementMode(MOVE_Walking);
    auto* Wall=Box(FVector(-70,0,100),FVector(10,300,100));
    const float Before=ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute());
    const int32 Uses=Skills->GetSkillUses(Definition);
    ASC->TryActivateAbility(Spec->Handle);
    TestFalse(TEXT("first release is neutral backward intent and nearby wall rejects before payment"),Window->IsWindowAuthority());
    TestEqual(TEXT("blocked admission spends no stamina"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),Before);
    TestEqual(TEXT("blocked admission earns no use"),Skills->GetSkillUses(Definition),Uses);
    TArray<FVector> DiagnosticPath;
    FName StopReason;
    FHitResult StoppingHit;
    TestFalse(TEXT("explicit-character query remains usable after rejected window cleanup"),
        UMythicGA_AbilityWindow::QueryAbilityEscapePath(Avatar,-FVector::ForwardVector,320,DiagnosticPath,StopReason,StoppingHit));
    TestTrue(TEXT("diagnostic retains the exact corridor obstruction without a window avatar"),
        DiagnosticPath.IsEmpty() && StopReason==TEXT("CorridorBlocked") && StoppingHit.GetActor()==Wall);
    auto* WallShape=CastChecked<UPrimitiveComponent>(Wall->GetRootComponent());
    Capsule->IgnoreComponentWhenMoving(WallShape,true);
    TestTrue(TEXT("admission honors the same component ignore as actual character movement"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    Capsule->IgnoreComponentWhenMoving(WallShape,false);
    const float AfterIgnored=ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute());
    ASC->TryActivateAbility(Spec->Handle);
    TestFalse(TEXT("removing the movement ignore restores wall rejection"),Window->IsWindowAuthority());
    TestEqual(TEXT("restored wall rejection cannot pay again"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),AfterIgnored);
    Wall->Destroy();
    TestTrue(TEXT("neutral escape admits after obstruction removal"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    TestTrue(TEXT("released prior lateral input does not replay"),Window->GetMovementIntentDirection(true).Equals(-FVector::ForwardVector,0.001));
    const FGuid DriftAction=Window->BeginAbilityAction(TEXT("Escape"));
    Avatar->SetActorLocation(Start+FVector(10,0,0));
    TestFalse(TEXT("postcommit origin drift rejects rather than rebasing endpoint"),Window->StartPreparedUtilityEscape(DriftAction,Window->EscapePreparation));
    TestTrue(TEXT("failed start has one truthful native receipt"),Receipts.Num()==2 && Receipts.Last().Outcome==EMythicAbilityMovementOutcome::StartFailed);
    TestFalse(TEXT("failed start consumed the stale operation"),Window->StartPreparedUtilityEscape(DriftAction,Window->EscapePreparation));
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);

    Avatar->SetActorLocation(Start); Move->StopMovementImmediately(); Move->SetMovementMode(MOVE_Walking);
    bool bMovementCallback=false,bNestedActivation=false;
    const auto Reentry=ASC->OnAbilityMovementStarted.AddLambda([&](auto* Ability,const FVector&) {
        if (Ability!=Window) return;
        bMovementCallback=true; Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        bNestedActivation=ASC->TryActivateAbility(Spec->Handle);
    });
    TestTrue(TEXT("callback cancellation fixture admits through GAS"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    const FGuid ReentryAction=Window->BeginAbilityAction(TEXT("Escape"));
    TestFalse(TEXT("movement-start cancellation cannot continue arming the task"),Window->StartPreparedUtilityEscape(ReentryAction,Window->EscapePreparation));
    ASC->OnAbilityMovementStarted.Remove(Reentry);
    TestTrue(TEXT("real movement-start callback ran"),bMovementCallback);
    TestFalse(TEXT("same instance cannot activate inside movement-start callback"),bNestedActivation);
    TestTrue(TEXT("start callback cancellation produces one receipt"),Receipts.Num()==3 && Receipts.Last().Outcome==EMythicAbilityMovementOutcome::Cancelled);

    ADD_LATENT_AUTOMATION_COMMAND(FUtilityTravelCommand(this,Game,Avatar,Window,ASC,Spec->Handle,Start));
    bLatentOwnsWorld=true;
    return true;
}
#endif
