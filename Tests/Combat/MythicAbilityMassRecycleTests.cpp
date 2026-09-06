#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AIController.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "Player/MythicFactionStandingComponent.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ObjectKey.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityMassRecipientRecycleTest,"Mythic.Combat.Abilities.MassRecipientRecycle",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityMassRecipientRecycleTest::RunTest(const FString&) {
    const int32 OriginalContexts=GEngine->GetWorldContexts().Num();
    TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>(GEngine)); Game->InitializeStandalone();
    UWorld* World=Game->GetWorld(); bool bDestroyed=false;
    ON_SCOPE_EXIT { if (!bDestroyed) DestroyOwnedAbilityTestWorld(Game.Get()); };
    if (!TestTrue(TEXT("fixture owns a separate authoritative game world"),World && World->WorldType==EWorldType::Game && World->GetNetMode()==NM_Standalone && World->GetGameInstance()==Game.Get())) return false;
    // Initialization used to misclassify prewarmed runtime actors as map objects,
    // producing static NetGUIDs that remote clients could never resolve.
    TArray<TWeakObjectPtr<AActor>> EarlyRuntimeActors;
    for (TActorIterator<AActor> It(World); It; ++It) {
        if (It->IsA<AMythicNPCCharacter>() || It->IsA<AMythicLivingWorldReplicator>()) {
            if (!TestFalse(TEXT("prewarmed runtime body is never loaded as a map actor"),It->bNetLoadOnClient)) return false;
            EarlyRuntimeActors.Add(*It);
        }
    }
    if (!TestTrue(TEXT("fixture exercises subsystem actors created before level initialization"),!EarlyRuntimeActors.IsEmpty())) return false;
    World->InitializeActorsForPlay(FURL());
    for (const auto Actor : EarlyRuntimeActors) {
        if (!TestTrue(TEXT("level initialization preserves dynamic runtime network identity"),Actor.IsValid() && !Actor->IsNetStartupActor() && !Actor->IsNameStableForNetworking())) return false;
    }
    auto* LWS=Game->GetSubsystem<UMythicLivingWorldSubsystem>();
    auto* Mass=World->GetSubsystem<UMassEntitySubsystem>();
    auto* PresentationRegistry=World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!TestTrue(TEXT("real Mass, living world and presentation registries are available"),LWS && Mass && PresentationRegistry && LWS->GetSettings() && LWS->GetFactionDatabase() && LWS->GetTerritoryGrid() && LWS->GetPersistentNPCRegistry())) return false;
    const auto* Settings=LWS->GetSettings(); auto* Identities=LWS->GetPersistentNPCRegistry();
    if (!TestTrue(TEXT("production embodiment pooling is enabled"),Settings->bEnableEmbodimentPooling && Settings->EmbodimentPoolMaxPerClass>0)) return false;
    UClass* ActorClass=Settings->EmbodiedNPCClass.IsNull() ? AMythicNPCCharacter::StaticClass() : Settings->EmbodiedNPCClass.LoadSynchronous();
    if (!TestNotNull(TEXT("configured production NPC class resolves"),ActorClass)) return false;
    FMythicFactionId Faction; FMythicFactionData FactionData;
    LWS->GetFactionDatabase()->ForEachAliveFaction([&](FMythicFactionId Id,const FMythicFactionData& Data) {
        if ((!Faction.IsValid() || (FactionData.bCanNegotiate && !Data.bCanNegotiate) || (FactionData.bCanNegotiate==Data.bCanNegotiate && Id.Index<Faction.Index))) { Faction=Id; FactionData=Data; }
    });
    if (!TestTrue(TEXT("authored living faction exists"),Faction.IsValid())) return false;
    auto* Player=World->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player); auto* Avatar=InitializeAbilityTestAvatar(Player);
    if (!TestNotNull(TEXT("production player avatar initializes"),Avatar)) return false;
    if (FactionData.bCanNegotiate) Player->GetFactionStanding()->SetStanding(Faction,Player->GetFactionStanding()->GetHostileThreshold());
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Skills=Player->GetSkillComponent();
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* A=MakeAbilityWindowTestDefinition(Game.Get(),FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    auto* B=MakeAbilityWindowTestDefinition(Game.Get(),FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.ObsidianMaw")));
    RegisterAbilityTestDefinition(Skills,A,true,false); RegisterAbilityTestDefinition(Skills,B,true,false); Skills->GrantSlot();
    Skills->ServerEquipSkill(0,A); Skills->ServerEquipSkill(1,B);
    auto* SpecA=FindAbilityTestSpec(ASC,A); auto* SpecB=FindAbilityTestSpec(ASC,B);
    if (!TestTrue(TEXT("both probes use real granted Q/E specs"),SpecA && SpecB)) return false;
    auto* WindowA=Cast<UMythicGA_AbilityWindow>(SpecA->GetPrimaryInstance()); auto* WindowB=Cast<UMythicGA_AbilityWindow>(SpecB->GetPrimaryInstance());
    if (!TestTrue(TEXT("both probes are production windows"),WindowA && WindowB)) return false;
    auto& Manager=Mass->GetMutableEntityManager();
    const UScriptStruct* Composition[]={FMythicIdentityFragment::StaticStruct(),FMythicScheduleFragment::StaticStruct(),FMythicSignificanceFragment::StaticStruct(),FMythicPersonalityFragment::StaticStruct(),FMythicPsychodynamicFragment::StaticStruct(),FMythicSocialFragment::StaticStruct(),FMythicNPCTag::StaticStruct(),FMythicHydratedTag::StaticStruct()};
    const auto Archetype=Manager.CreateArchetype(MakeArrayView(Composition));
    const FVector Position=Avatar->GetActorLocation()+FVector(400,0,0);
    const auto CreateEntity=[&](uint32 Seed) {
        TArray<FMassEntityHandle> Entities;
        {
            auto Creation=Manager.BatchCreateEntities(Archetype,1,Entities);
            auto& Identity=Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entities[0]);
            Identity.NameSeed=Seed; Identity.EntityId=Identities->AllocateEntityIdentity(Seed,EMythicEntityIdentityProvenance::Runtime);
            Identity.Faction=Identity.TrueFaction=Faction; Identity.Cell=LWS->GetTerritoryGrid()->WorldToCell(Position); Identity.RoleTag=TAG_NPC_ROLE_BANDIT;
            auto& Schedule=Manager.GetFragmentDataChecked<FMythicScheduleFragment>(Entities[0]); Schedule.HomeCell=Schedule.WorkCell=Identity.Cell; Schedule.Phase=EMythicSchedulePhase::Idle;
            Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entities[0]).Tier=EMythicSignificanceTier::Tier2_Cognitive;
            Manager.GetFragmentDataChecked<FMythicPersonalityFragment>(Entities[0])=FMythicNPCGenerator::GeneratePersonality(Seed,FactionData.Ideology,Identity.RoleTag);
        }
        return Entities[0];
    };
    FMassEntityHandle EA,EB; TWeakObjectPtr<AMythicNPCCharacter> Recipient; FMassEntityHandle EmbodiedEntity;
    const auto CleanupEntities=[&]() {
        if (WindowA->IsWindowAuthority()) WindowA->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        if (WindowB->IsWindowAuthority()) WindowB->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        if (Recipient.IsValid()) { LWS->ReleaseEmbodiedActor(EmbodiedEntity,Recipient.Get()); Recipient.Reset(); }
        if (Manager.IsEntityValid(EA)) Manager.DestroyEntity(EA);
        if (Manager.IsEntityValid(EB)) Manager.DestroyEntity(EB);
    };
    bool bCleaned=false; ON_SCOPE_EXIT { if (!bCleaned) CleanupEntities(); };
    const auto Embody=[&](FMassEntityHandle Entity) {
        Recipient=LWS->AcquireEmbodiedActor(ActorClass,Position,FRotator::ZeroRotator); EmbodiedEntity=Entity;
        if (!Recipient.IsValid()) return false;
        Recipient->InitializeFromMassEntity(Entity);
        if (!Recipient->ActivatePreparedEmbodiment()) return false;
        LWS->RegisterEmbodiedActor(Entity,Recipient.Get()); Manager.AddTagToEntity(Entity,FMythicCognitiveTag::StaticStruct());
        return true;
    };
    auto Evidence=MakeShared<FJsonObject>(); Evidence->SetNumberField(TEXT("schema_version"),1); Evidence->SetStringField(TEXT("native_test"),TEXT("Mythic.Combat.Abilities.MassRecipientRecycle"));
    auto WorldProof=MakeShared<FJsonObject>(); WorldProof->SetStringField(TEXT("type"),TEXT("Game")); WorldProof->SetStringField(TEXT("net_mode"),TEXT("Standalone"));
    for (const auto* Key:{TEXT("authority"),TEXT("owned_transient"),TEXT("not_editor_world")}) WorldProof->SetBoolField(Key,true);
    Evidence->SetObjectField(TEXT("world"),WorldProof);
    bool bProof=true;
    const auto Record=[&](const TSharedPtr<FJsonObject>& Object,const TCHAR* Key,bool Value) { Object->SetBoolField(Key,Value); bProof &= TestTrue(Key,Value); };
    const auto Describe=[&](FMassEntityHandle Entity,AMythicNPCCharacter* Actor) {
        auto Object=MakeShared<FJsonObject>(); auto* Brain=Actor->FindComponentByClass<UMythicCognitiveBrainComponent>(); auto* Presentation=Actor->GetEntityPresentationComponent_Implementation();
        const auto Identity=Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity).EntityId; const auto Instance=Presentation->GetPresentationInstance();
        Object->SetStringField(TEXT("actor_pointer"),FString::Printf(TEXT("%p"),Actor)); Object->SetStringField(TEXT("actor_object_key"),FString::Printf(TEXT("%u"),GetTypeHash(FObjectKey(Actor))));
        Object->SetStringField(TEXT("canonical_id"),Identity.ToDebugString()); Object->SetStringField(TEXT("presentation_instance"),Instance.ToDebugString());
        auto Handle=MakeShared<FJsonObject>(); Handle->SetNumberField(TEXT("index"),Entity.Index); Handle->SetNumberField(TEXT("serial"),Entity.SerialNumber); Object->SetObjectField(TEXT("mass_handle"),Handle);
        TArray<TSharedPtr<FJsonValue>> Fragments,Tags;
        const auto AddFragment=[&](const TCHAR* Name,bool Present) { bProof &= TestTrue(Name,Present); if (Present) Fragments.Add(MakeShared<FJsonValueString>(Name)); };
        AddFragment(TEXT("FMythicIdentityFragment"),Manager.GetFragmentDataPtr<FMythicIdentityFragment>(Entity)!=nullptr);
        AddFragment(TEXT("FMythicScheduleFragment"),Manager.GetFragmentDataPtr<FMythicScheduleFragment>(Entity)!=nullptr);
        AddFragment(TEXT("FMythicSignificanceFragment"),Manager.GetFragmentDataPtr<FMythicSignificanceFragment>(Entity)!=nullptr);
        AddFragment(TEXT("FMythicPersonalityFragment"),Manager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity)!=nullptr);
        AddFragment(TEXT("FMythicPsychodynamicFragment"),Manager.GetFragmentDataPtr<FMythicPsychodynamicFragment>(Entity)!=nullptr);
        AddFragment(TEXT("FMythicSocialFragment"),Manager.GetFragmentDataPtr<FMythicSocialFragment>(Entity)!=nullptr);
        const FMassEntityView View(Manager,Entity);
        const auto AddTag=[&](const TCHAR* Name,bool Present) { bProof &= TestTrue(Name,Present); if (Present) Tags.Add(MakeShared<FJsonValueString>(Name)); };
        AddTag(TEXT("FMythicNPCTag"),View.HasTag<FMythicNPCTag>()); AddTag(TEXT("FMythicHydratedTag"),View.HasTag<FMythicHydratedTag>()); AddTag(TEXT("FMythicCognitiveTag"),View.HasTag<FMythicCognitiveTag>());
        Object->SetArrayField(TEXT("fragments"),Fragments); Object->SetArrayField(TEXT("tags"),Tags);
        Record(Object,TEXT("mass_valid"),Manager.IsEntityValid(Entity)); Record(Object,TEXT("identity_registered"),Identities->ContainsEntityIdentity(Identity));
        Record(Object,TEXT("brain_matches_mass"),Brain && Brain->GetSourceEntity()==Entity); Record(Object,TEXT("lookup_matches_actor"),LWS->FindEmbodiedActor(Entity)==Actor);
        Record(Object,TEXT("presentation_resolves_actor"),PresentationRegistry->ResolvePresentationComponent(Instance)==Presentation);
        FMythicEntityId Resolved; Record(Object,TEXT("presentation_identity_matches_mass"),PresentationRegistry->ResolveAuthorityEntity(Instance,Resolved) && Resolved==Identity && Presentation->GetAuthorityEntityId()==Identity);
        auto* Life=UMythicLifeComponent::FindHealthComponent(Actor); auto* TargetASC=Actor->GetAbilitySystemComponent(); auto* AI=Cast<AAIController>(Actor->GetController());
        Record(Object,TEXT("combat_ready"),Life && Life->IsInitialized() && TargetASC && TargetASC->GetAvatarActor()==Actor && TargetASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute())>0 && AI && AI->GetTeamAttitudeTowards(*Avatar)==ETeamAttitude::Hostile);
        return Object;
    };
    EA=CreateEntity(137); if (!TestTrue(TEXT("first real Mass entity hydrates through production pooling"),Embody(EA))) return false;
    auto* OriginalActor=Recipient.Get(); const TWeakObjectPtr<AMythicNPCCharacter> OriginalWeak=OriginalActor; const FObjectKey OriginalKey(OriginalActor);
    const auto OldInstance=OriginalActor->GetEntityPresentationComponent_Implementation()->GetPresentationInstance();
    const auto OldIdentity=Manager.GetFragmentDataChecked<FMythicIdentityFragment>(EA).EntityId;
    Evidence->SetObjectField(TEXT("before"),Describe(EA,OriginalActor));
    if (!TestTrue(TEXT("first action activates through real GAS"),ASC->TryActivateAbility(SpecA->Handle) && WindowA->IsWindowAuthority())) return false;
    auto Actions=MakeShared<FJsonObject>(); Record(Actions,TEXT("old_recipient_retained_before_release"),WindowA->TryVisitAbilityTarget(TEXT("RecycleProbe"),OriginalActor,1));
    const FGuid H1=WindowA->BeginAbilityAction(TEXT("Probe")); if (!TestTrue(TEXT("first unspent action exists"),H1.IsValid())) return false;
    const FGuid ActivationA=WindowA->GetWindowActivationId();
    LWS->ReleaseEmbodiedActor(EA,OriginalActor); Recipient.Reset();
    auto Release=MakeShared<FJsonObject>(); Record(Release,TEXT("actor_alive"),OriginalWeak.IsValid()); Record(Release,TEXT("lookup_removed"),LWS->FindEmbodiedActor(EA)==nullptr);
    if (!OriginalWeak.IsValid()) return false;
    auto* ReleasedBrain=OriginalActor->FindComponentByClass<UMythicCognitiveBrainComponent>();
    auto* ReleasedPresentation=OriginalActor->GetEntityPresentationComponent_Implementation();
    if (!TestTrue(TEXT("pooled body retains its brain and presentation components"),ReleasedBrain && ReleasedPresentation)) return false;
    Record(Release,TEXT("presentation_inactive"),!ReleasedPresentation->GetPresentationInstance().IsValid());
    Record(Release,TEXT("old_instance_unresolvable"),PresentationRegistry->ResolvePresentationComponent(OldInstance)==nullptr);
    Record(Release,TEXT("brain_source_unset"),!ReleasedBrain->GetSourceEntity().IsSet());
    Manager.DestroyEntity(EA); Record(Release,TEXT("old_mass_destroyed"),!Manager.IsEntityValid(EA)); Evidence->SetObjectField(TEXT("release"),Release);
    EB=CreateEntity(139); if (!TestTrue(TEXT("second real Mass entity hydrates through production pooling"),Embody(EB))) return false;
    auto* Reused=Recipient.Get();
    if (!TestTrue(TEXT("pool reuses the exact living UObject and weak identity"),Reused==OriginalActor && OriginalWeak.Get()==Reused && FObjectKey(Reused)==OriginalKey)) return false;
    const auto NewInstance=Reused->GetEntityPresentationComponent_Implementation()->GetPresentationInstance();
    bProof &= TestTrue(TEXT("reused body has new Mass identity and public incarnation"),EA!=EB && OldIdentity!=Manager.GetFragmentDataChecked<FMythicIdentityFragment>(EB).EntityId && NewInstance!=OldInstance);
    Evidence->SetObjectField(TEXT("after"),Describe(EB,Reused));
    if (!TestTrue(TEXT("fresh action activates with its own cooldown identity"),ASC->TryActivateAbility(SpecB->Handle) && WindowB->IsWindowAuthority())) return false;
    const FGuid H2=WindowB->BeginAbilityAction(TEXT("Probe")); if (!TestTrue(TEXT("fresh unspent action exists"),H2.IsValid())) return false;
    Actions->SetStringField(TEXT("old_activation"),ActivationA.ToString()); Actions->SetStringField(TEXT("new_activation"),WindowB->GetWindowActivationId().ToString()); Actions->SetStringField(TEXT("old_handle"),H1.ToString()); Actions->SetStringField(TEXT("new_handle"),H2.ToString());
    Record(Actions,TEXT("old_activation_live_at_attempt"),WindowA->IsWindowAuthority() && WindowA->GetWindowActivationId()==ActivationA);
    const auto* OldGroup=WindowA->ActionGroups.Find(H1); Record(Actions,TEXT("old_handle_live_at_attempt"),OldGroup && (*OldGroup)->Activation==ActivationA);
    Record(Actions,TEXT("old_action_unspent_before_attempt"),OldGroup && (*OldGroup)->AllTargets.IsEmpty() && (*OldGroup)->ClaimedTargets.Num()==1 && (*OldGroup)->ClaimedTargets[0].IsEmpty());
    Record(Actions,TEXT("new_target_other_gates_valid"),WindowB->IsValidAbilityTarget(Reused,false,true));
    auto* TargetASC=Reused->GetAbilitySystemComponent(); TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetDodgeChanceAttribute(),0);
    const auto Health=[&]() { return TargetASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()); };
    const auto Shield=[&]() { return TargetASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute()); };
    const float HealthBefore=Health(),ShieldBefore=Shield(); const auto Stale=WindowA->ApplyAbilityAction(Reused,H1);
    Actions->SetBoolField(TEXT("old_accepted"),Stale.bAccepted); Actions->SetNumberField(TEXT("old_health_lost"),Stale.HealthLost); Actions->SetNumberField(TEXT("old_shield_absorbed"),Stale.ShieldAbsorbed);
    Actions->SetNumberField(TEXT("health_before_stale"),HealthBefore); Actions->SetNumberField(TEXT("health_after_stale"),Health()); Actions->SetNumberField(TEXT("shield_before_stale"),ShieldBefore); Actions->SetNumberField(TEXT("shield_after_stale"),Shield());
    bProof &= TestTrue(TEXT("old unspent action rejects recycled incarnation without resource damage"),!Stale.bAccepted && Stale.HealthLost==0 && Stale.ShieldAbsorbed==0 && Health()==HealthBefore && Shield()==ShieldBefore);
    const float FreshBefore=Health(); const auto Fresh=WindowB->ApplyAbilityAction(Reused,H2);
    Actions->SetBoolField(TEXT("new_accepted"),Fresh.bAccepted); Actions->SetNumberField(TEXT("new_health_lost"),Fresh.HealthLost); Actions->SetNumberField(TEXT("health_before_fresh"),FreshBefore); Actions->SetNumberField(TEXT("health_after_fresh"),Health());
    bProof &= TestTrue(TEXT("fresh action damages the same Mass recipient through canonical settlement"),Fresh.bAccepted && Fresh.HealthLost>0 && Health()>0 && FMath::IsNearlyEqual(FreshBefore-Health(),Fresh.HealthLost)); Evidence->SetObjectField(TEXT("actions"),Actions);
    CleanupEntities(); bCleaned=true;
    auto Cleanup=MakeShared<FJsonObject>(); Record(Cleanup,TEXT("both_mass_invalid"),!Manager.IsEntityValid(EA) && !Manager.IsEntityValid(EB)); Record(Cleanup,TEXT("both_lookups_absent"),!LWS->FindEmbodiedActor(EA) && !LWS->FindEmbodiedActor(EB));
    Record(Cleanup,TEXT("both_instances_unresolvable"),!PresentationRegistry->ResolvePresentationComponent(OldInstance) && !PresentationRegistry->ResolvePresentationComponent(NewInstance)); Record(Cleanup,TEXT("abilities_ended"),!WindowA->IsActive() && !WindowB->IsActive());
    DestroyOwnedAbilityTestWorld(Game.Get()); bDestroyed=true;
    Record(Cleanup,TEXT("world_destroyed"),!GEngine->GetWorldContextFromWorld(World)); Record(Cleanup,TEXT("world_contexts_restored"),GEngine->GetWorldContexts().Num()==OriginalContexts); Evidence->SetObjectField(TEXT("cleanup"),Cleanup);
    if (bProof) { FString Json; FJsonSerializer::Serialize(Evidence,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json)); AddInfo(TEXT("ABILITY_MASS_RECYCLE_EVIDENCE:")+Json); }
    return bProof;
}
#endif
