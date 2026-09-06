#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Combat/MythicUtilityWindowTestProbe.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicTags_GAS.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityUtilityActionTest,"Mythic.Combat.Abilities.UtilityAction",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityUtilityActionTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Avatar=InitializeAbilityTestAvatar(Player);
    if (!TestNotNull(TEXT("production local avatar initializes"),Avatar)) return false;
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Skills=Player->GetSkillComponent();
    auto* Definition=MakeAbilityWindowTestDefinition(Game,FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    Definition->RuntimeAbility=UMythicUtilityWindowTestProbe::StaticClass();
    Definition->ReusePolicy=EMythicAbilityReusePolicy::NoTimedCooldown;
    Definition->CooldownSeconds=0; Definition->CooldownTag={}; Definition->CooldownEffect=nullptr;
    FMythicAbilityActionProfile Utility;
    Utility.ActionId=TEXT("Escape"); Utility.Work=EMythicAbilityActionWork::Utility; Utility.bScaleWithPrimaryStats=false; Utility.MaximumTargets=1;
    Utility.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Target.Self")));
    Utility.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.Movement")));
    Utility.Parameters={{TEXT("Distance"),320,40,600},{TEXT("Transit"),0.22f,0.05f,1.0f}};
    TArray<FText> Errors;
    TestTrue(TEXT("utility requires no damage or status GE"),Utility.Validate(Errors));
    auto Invalid=Utility; Invalid.MinimumDamage=Invalid.MaximumDamage=1; Errors.Reset();
    TestFalse(TEXT("utility cannot disguise authored damage"),Invalid.Validate(Errors));
    Invalid=Utility; Invalid.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.Damage"))); Errors.Reset();
    TestFalse(TEXT("utility cannot opt into damage itemization with a false facet"),Invalid.Validate(Errors));
    Invalid=Utility; const auto DuplicateParameter=Invalid.Parameters[0]; Invalid.Parameters.Add(DuplicateParameter); Errors.Reset();
    TestFalse(TEXT("duplicate parameters cannot resolve by row order"),Invalid.Validate(Errors));
    Definition->Actions={Utility}; RegisterAbilityTestDefinition(Skills,Definition,true,false); Skills->ServerEquipSkill(0,Definition);
    auto* Spec=FindAbilityTestSpec(ASC,Definition);
    auto* Window=Spec ? Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("utility uses the actual equipped GAS window"),Window)) return false;
    CastChecked<UMythicUtilityWindowTestProbe>(Window)->bPrepareEscape=false;
    Window->bRequiresCameraAim=false;
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* Affixes=Player->FindComponentByClass<UMythicAffixApplicationComponent>();
    if (!TestNotNull(TEXT("production affix source exists"),Affixes)) return false;
    {
        TGuardValue<bool> Quarantine(Affixes->bApplicationQuarantined,true);
        ASC->TryActivateAbility(Spec->Handle);
        TestFalse(TEXT("quarantined utility source cannot commit"),Window->IsWindowAuthority());
        TestEqual(TEXT("quarantined utility spends no resource"),ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()),100.0f);
        TestEqual(TEXT("quarantined utility grants no practice"),Skills->GetSkillUses(Definition),0);
    }
    int32 DamageEvents=0;
    const auto Listener=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_PRE).AddLambda([&](const FGameplayEventData*){++DamageEvents;});
    ON_SCOPE_EXIT { ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_PRE).Remove(Listener); };
    TestTrue(TEXT("solo self utility commits through GAS"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    if (!TestTrue(TEXT("utility owns a frozen source without offensive capture work"),Window->ActionSource.IsValid()
        && Window->ActionSource->Stats.IsEmpty() && Window->ActionSource->Captures.IsEmpty() && !Window->ActionSource->EffectDefinitions)) return false;
    const auto BeforeEffects=ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
    const FGuid Handle=Window->BeginAbilityAction(TEXT("Escape"));
    TestTrue(TEXT("one utility group is admitted"),Handle.IsValid());
    TestEqual(TEXT("utility builds no outgoing or calculation GE"),ASC->GetActiveEffects(FGameplayEffectQuery()).Num(),BeforeEffects);
    if (const auto* Group=Window->ActionGroups.Find(Handle)) TestFalse(TEXT("utility stores no offensive application spec"),(*Group)->Application.IsValid());
    float Value=0;
    Definition->Actions[0].Parameters[0].Value=500;
    TestTrue(TEXT("parameter remains frozen despite authoring object mutation"),Window->TryGetUtilityActionParameter(Handle,TEXT("Distance"),Value) && Value==320);
    TestFalse(TEXT("missing utility parameter has no fabricated fallback"),Window->TryGetUtilityActionParameter(Handle,TEXT("Missing"),Value));
    TestFalse(TEXT("utility handle cannot deliver an offensive hit"),Window->ApplyAbilityAction(Avatar,Handle).bAccepted);
    TestEqual(TEXT("utility emits no Dmg.Pre"),DamageEvents,0);
    Window->EndAbilityAction(Handle);
    TestFalse(TEXT("released utility handle cannot read or settle work"),Window->TryGetUtilityActionParameter(Handle,TEXT("Distance"),Value));
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    Definition->Actions[0]=Utility;
    const auto Attribute=UMythicAttributeSet_Offense::GetStatusBuildupMultiplierAttribute();
    Definition->Actions[0].SourceAttributes={Attribute};
    ASC->SetNumericAttributeBase(Attribute,2.0f);
    auto* Effect=NewObject<UGameplayEffect>(); Effect->DurationPolicy=EGameplayEffectDurationType::Infinite;
    auto& Modifier=Effect->Modifiers.AddDefaulted_GetRef(); Modifier.Attribute=Attribute;
    Modifier.ModifierOp=EGameplayModOp::Additive; Modifier.ModifierMagnitude=FGameplayEffectModifierMagnitude(FScalableFloat(3));
    const auto Active=ASC->ApplyGameplayEffectToSelf(Effect,1,ASC->MakeEffectContext());
    TestTrue(TEXT("declared source-stat utility commits"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    const FGuid Captured=Window->BeginAbilityAction(TEXT("Escape"));
    ASC->RemoveActiveGameplayEffect(Active); ASC->SetNumericAttributeBase(Attribute,20.0f);
    TestTrue(TEXT("declared source stat survives effect removal and live base changes"),Window->TryReadUtilityActionSourceAttribute(Captured,Attribute,Value) && FMath::IsNearlyEqual(Value,5.0f));
    TestFalse(TEXT("undeclared source attribute cannot fall back to live stats"),Window->TryReadUtilityActionSourceAttribute(Captured,UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),Value));
    {
        TGuardValue<int32> NativeCallbacks(Window->UtilityCallbackDepth,1);
        TestFalse(TEXT("CDO activation check respects native callback on the live instance"),GetDefault<UMythicAbilityWindowTestProbe>()->CanActivateAbility(Spec->Handle,ASC->AbilityActorInfo.Get()));
    }
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    auto* Registry=Game->GetSubsystem<UMythicStatusRegistry>();
    auto* Burn=Registry ? const_cast<UMythicStatusEffectDefinition*>(Registry->FindStatus(FGameplayTag::RequestGameplayTag(TEXT("Status.Type.Burn")))) : nullptr;
    if (!TestNotNull(TEXT("canonical Burn exists for removal callback regression"),Burn)) return false;
    TGuardValue<bool> AllowBurn(Burn->bAllowSourceGroupRemoval,true);
    auto* Source=Game->GetWorld()->SpawnActor<AActor>();
    TestTrue(TEXT("native canonical status creates a real source group"),UMythicStatusRegistry::ApplyStatusEffect(ASC,Burn,Source,Source,5));
    auto Shed=Utility; Shed.ActionId=TEXT("Shed"); Shed.Facets.Reset();
    Shed.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Target.Self")));
    Shed.Facets.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.RemoveStatus")));
    Shed.Parameters={{TEXT("RemnantMaximum"),3,0,3}};
    Definition->Actions.Add(Shed);
    auto* Probe=CastChecked<UMythicUtilityWindowTestProbe>(Window); Probe->bReserveBurn=true;
    TestTrue(TEXT("status utility reserves through precommit validation"),ASC->TryActivateAbility(Spec->Handle) && Probe->StatusReservation.IsValid());
    const FGuid ShedHandle=Window->BeginAbilityAction(TEXT("Shed")),OldReservation=Probe->StatusReservation;
    bool bObservedRemoval=false,bNestedActivation=false;
    const auto Reentry=ASC->OnAnyGameplayEffectRemovedDelegate().AddLambda([&](const FActiveGameplayEffect& Removed) {
        if (Removed.Spec.GetContext().GetSourceObject()!=Burn) return;
        bObservedRemoval=true; Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
        bNestedActivation=ASC->TryActivateAbility(Spec->Handle);
    });
    const auto Removal=Window->ConsumeUtilityStatusGroup(ShedHandle,OldReservation);
    ASC->OnAnyGameplayEffectRemovedDelegate().Remove(Reentry);
    TestTrue(TEXT("real GAS removal callback executes exactly one successful operation"),bObservedRemoval && Removal.bRemoved && Removal.ActionGroupId==ShedHandle);
    TestFalse(TEXT("same instance cannot reactivate inside removal callback"),bNestedActivation);
    TestFalse(TEXT("cancelled removal continuation cannot reuse old handles"),Window->ConsumeUtilityStatusGroup(ShedHandle,OldReservation).bRemoved);
    TestTrue(TEXT("fresh activation succeeds after native callback unwinds"),ASC->TryActivateAbility(Spec->Handle) && Window->IsWindowAuthority());
    Window->EndAbilityWindow(EMythicAbilityWindowEnd::Cancelled);
    return true;
}
#endif
