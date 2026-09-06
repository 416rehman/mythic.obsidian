#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "GAS/Effects/MythicStatusGroupReservation.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "GameplayEffect.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicStatusGroupReservationTest,"Mythic.Combat.StatusGroupReservation",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicStatusGroupReservationTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    auto* Player=World->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Avatar=InitializeAbilityTestAvatar(Player);
    auto* ASC=Player->GetMythicAbilitySystemComponent();
    auto* Registry=Game->GetSubsystem<UMythicStatusRegistry>();
    if (!TestNotNull(TEXT("production recipient initializes"),Avatar) || !TestNotNull(TEXT("canonical registry initializes"),Registry)) return false;
    auto CopyStatus=[&](const TCHAR* Name) {
        const auto* Original=Registry->FindStatus(FGameplayTag::RequestGameplayTag(Name));
        auto* Copy=Original ? DuplicateObject<UMythicStatusEffectDefinition>(Original,Game) : nullptr;
        if (Copy) Copy->bAllowSourceGroupRemoval=true;
        return Copy;
    };
    auto* Burn=CopyStatus(TEXT("Status.Type.Burn"));
    auto* Poison=CopyStatus(TEXT("Status.Type.Poison"));
    auto* Slow=CopyStatus(TEXT("Status.Type.Slow"));
    if (!TestNotNull(TEXT("Burn definition exists"),Burn) || !TestNotNull(TEXT("Poison definition exists"),Poison)
        || !TestNotNull(TEXT("Slow definition exists"),Slow)) return false;
    auto* SourceA=World->SpawnActor<AActor>();
    auto* SourceB=World->SpawnActor<AActor>();
    auto Apply=[&](UMythicStatusEffectDefinition* Definition,AActor* Source,float Seconds) {
        return UMythicStatusRegistry::ApplyStatusEffect(ASC,Definition,Source,Source,Seconds);
    };
    auto Find=[&](const UMythicStatusEffectDefinition* Definition,const AActor* Source) {
        for (const auto Handle:ASC->GetActiveEffects(FGameplayEffectQuery{})) {
            const auto* Effect=ASC->GetActiveGameplayEffect(Handle);
            if (Effect && Effect->Spec.GetContext().GetSourceObject()==Definition && Effect->Spec.GetContext().GetInstigator()==Source) return Handle;
        }
        return FActiveGameplayEffectHandle{};
    };
    auto Clear=[&]() { ASC->RemoveActiveEffects(FGameplayEffectQuery{}); };
    const TArray<const UMythicStatusEffectDefinition*> Priority={Burn,Poison,Slow};
    TestFalse(TEXT("no affliction creates no reservation"),FMythicStatusGroupReservation::Reserve(ASC,Priority).IsValid());

    for (const float Roll:{7.0f,19.0f,31.0f}) {
        Burn->DamagePerTick.Min=Burn->DamagePerTick.Max=Roll;
        TestTrue(TEXT("uneven source A Burn applies"),Apply(Burn,SourceA,12));
    }
    TestTrue(TEXT("independent source B Burn applies"),Apply(Burn,SourceB,4));
    const auto HandleA=Find(Burn,SourceA),HandleB=Find(Burn,SourceB);
    const auto* A=ASC->GetActiveGameplayEffect(HandleA);
    const auto* B=ASC->GetActiveGameplayEffect(HandleB);
    if (!TestNotNull(TEXT("source A group exists"),A) || !TestNotNull(TEXT("source B group exists"),B)) return false;
    const float DamageA=A->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE,false,0);
    const FGameplayEffectSpec SpecB=B->Spec;
    const float StartB=B->StartWorldTime;
    auto First=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    auto Rival=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("two observers may reserve the same current group"),First.IsValid() && Rival.IsValid())) return false;
    FMythicStatusGroupRemoval Reentrant,ReentrantRival;
    const auto Reentry=ASC->OnAnyGameplayEffectRemovedDelegate().AddLambda([&](const FActiveGameplayEffect& Removed) {
        if (Removed.Handle==HandleA) { Reentrant=First->Consume(); ReentrantRival=Rival->Consume(); }
    });
    const auto Receipt=First->Consume();
    ASC->OnAnyGameplayEffectRemovedDelegate().Remove(Reentry);
    TestTrue(TEXT("one exact removal receipt is returned"),Receipt.bRemoved && Receipt.ReceiptId.IsValid());
    TestEqual(TEXT("all three source A members are reported"),Receipt.StackCount,3);
    TestEqual(TEXT("exact aggregate survives into descriptive receipt"),Receipt.DamagePerTick,DamageA);
    TestFalse(TEXT("reentry cannot consume twice"),Reentrant.bRemoved);
    TestFalse(TEXT("another lease cannot consume while GAS removal callbacks run"),ReentrantRival.bRemoved);
    TestFalse(TEXT("repeat cannot replay a receipt"),First->Consume().bRemoved);
    TestFalse(TEXT("rival reservation cannot remove another source"),Rival->Consume().bRemoved);
    TestNull(TEXT("source A exact handle was removed"),ASC->GetActiveGameplayEffect(HandleA));
    B=ASC->GetActiveGameplayEffect(HandleB);
    if (!TestNotNull(TEXT("source B exact handle remains"),B)) return false;
    TestTrue(TEXT("source B context remains identical"),B->Spec.GetContext().Get()==SpecB.GetContext().Get());
    TestEqual(TEXT("source B duration unchanged"),B->GetDuration(),SpecB.GetDuration());
    TestEqual(TEXT("source B start time unchanged"),B->StartWorldTime,StartB);
    TestEqual(TEXT("source B stack count unchanged"),B->Spec.GetStackCount(),SpecB.GetStackCount());
    TestEqual(TEXT("source B magnitude unchanged"),B->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE,false,0),SpecB.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE,false,0));

    for (int32 Index=0;Index<3;++Index) TestTrue(TEXT("restore capped A group"),Apply(Burn,SourceA,12));
    auto Capped=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("capped group is reservable"),Capped.IsValid())) return false;
    TestTrue(TEXT("at-cap refresh applies at the same world time"),Apply(Burn,SourceA,12));
    TestFalse(TEXT("at-cap refresh invalidates even when stack count and time are unchanged"),Capped->IsCurrent());
    TestFalse(TEXT("stale reservation cannot consume refreshed state"),Capped->Consume().bRemoved);

    auto SourceDies=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("valid group reserves before source death"),SourceDies.IsValid())) return false;
    SourceA->Destroy();
    TestTrue(TEXT("old recipient-owned affliction remains removable after source destruction"),SourceDies->Consume().bRemoved);
    TestNotNull(TEXT("other source remains after source destruction removal"),ASC->GetActiveGameplayEffect(HandleB));

    TestTrue(TEXT("longer Poison applies alongside shorter Burn"),Apply(Poison,SourceB,20));
    auto Ordered=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("ordered group reserves"),Ordered.IsValid())) return false;
    TestTrue(TEXT("family priority wins before cross-family duration"),Ordered->Consume().Definition==Burn);
    Clear();

    TestTrue(TEXT("source B Poison applies"),Apply(Poison,SourceB,5));
    Poison->bAllowSourceGroupRemoval=false;
    TestFalse(TEXT("definition must explicitly permit group removal"),FMythicStatusGroupReservation::Reserve(ASC,Priority).IsValid());
    Poison->bAllowSourceGroupRemoval=true;
    auto External=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("explicitly permitted source-isolated Poison reserves"),External.IsValid())) return false;
    Clear();
    TestFalse(TEXT("external cleanse invalidates the reservation"),External->Consume().bRemoved);

    auto* SourceC=World->SpawnActor<AActor>();
    Slow->ControlMagnitude.Min=Slow->ControlMagnitude.Max=0.2f;
    TestTrue(TEXT("first source shared Slow applies"),Apply(Slow,SourceB,5));
    const auto SlowB=Find(Slow,SourceB);
    const auto* SlowBEffect=ASC->GetActiveGameplayEffect(SlowB);
    if (!TestNotNull(TEXT("first source control group has an active effect"),SlowBEffect)) return false;
    TestTrue(TEXT("canonical Slow explicitly aggregates across sources"),SlowBEffect->Spec.Def->GetStackingType()==EGameplayEffectStackingType::AggregateByTarget);
    Slow->ControlMagnitude.Min=Slow->ControlMagnitude.Max=0.4f;
    TestTrue(TEXT("second source updates the shared control group"),Apply(Slow,SourceC,12));
    TestEqual(TEXT("second source shares the original Slow handle"),Find(Slow,SourceC),SlowB);
    TestFalse(TEXT("opt-in cannot make shared Slow source-isolated"),FMythicStatusGroupReservation::Reserve(ASC,Priority).IsValid());
    const auto* RemainingSlow=ASC->GetActiveGameplayEffect(SlowB);
    if (!TestNotNull(TEXT("rejected reservation leaves shared Slow intact"),RemainingSlow)) return false;
    Clear();
    TestTrue(TEXT("finite Poison for protection check applies"),Apply(Poison,SourceB,5));
    const auto* Protectable=ASC->GetActiveGameplayEffect(Find(Poison,SourceB));
    if (!TestNotNull(TEXT("protectable source group exists"),Protectable)) return false;
    FGameplayEffectSpec ProtectedSpec=Protectable->Spec;
    ProtectedSpec.AppendDynamicAssetTags(GAS_STATUS_GROUP_REMOVAL_PROTECTED.GetTag().GetSingleTagContainer());
    Clear();
    TestTrue(TEXT("explicitly protected finite group applies"),ASC->ApplyGameplayEffectSpecToSelf(ProtectedSpec).IsValid());
    TestFalse(TEXT("effect protection overrides definition permission"),FMythicStatusGroupReservation::Reserve(ASC,Priority).IsValid());
    Clear();
    TestTrue(TEXT("short-lived group applies"),Apply(Poison,SourceB,0.02f));
    auto Expiring=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    if (!TestTrue(TEXT("unexpired finite group reserves"),Expiring.IsValid())) return false;
    World->Tick(LEVELTICK_All,0.1f);
    TestFalse(TEXT("natural expiry cannot produce a later removal receipt"),Expiring->Consume().bRemoved);
    Clear();

    TestTrue(TEXT("final Poison applies"),Apply(Poison,SourceB,5));
    auto Incarnation=FMythicStatusGroupReservation::Reserve(ASC,Priority);
    auto* Presentation=Avatar->FindComponentByClass<UMythicEntityPresentationComponent>();
    if (!TestTrue(TEXT("recipient lease and real presentation exist"),Incarnation.IsValid() && Presentation)) return false;
    Presentation->AuthorityDeactivateEmbodiment();
    TestFalse(TEXT("recipient incarnation loss invalidates removal"),Incarnation->Consume().bRemoved);
    TestTrue(TEXT("invalidated consume does not change the recipient status store"),Find(Poison,SourceB).IsValid());
    Clear();
    return true;
}
#endif
