#include "Misc/AutomationTest.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Rewards/AbilityReward.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "GAS/Abilities/MythicGA_AbilityWindow.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityCharacterOwnershipTest,"Mythic.Progression.Skills.AbilityCharacterOwnership",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityCharacterOwnershipTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld(); auto* Player=World->SpawnActor<AMythicPlayerState>();
    InitializeAbilityTestPlayer(Player); auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    auto* Shipped=LoadObject<UMythicSkillDefinition>(nullptr,TEXT("/Game/Mythic/Progression/Abilities/DA_Ability_CinderLash.DA_Ability_CinderLash"));
    if (!TestNotNull(TEXT("the shipped definition supplies a real duration cooldown"),Shipped)) return false;
    const auto MakeDefinition=[&]() {
        auto* Definition=DuplicateObject<UMythicSkillDefinition>(Shipped,GetTransientPackage());
        Definition->RuntimeAbility.Reset(); Definition->Ability=UGameplayAbility::StaticClass();
        Definition->Acquisition=EMythicAbilityAcquisition::RewardOnly;
        Definition->Modifiers.Reset();
        for (const FName Id:{FName(TEXT("SavedChoiceA")),FName(TEXT("SavedChoiceB"))}) {
            auto& Modifier=Definition->Modifiers.AddDefaulted_GetRef();
            Modifier.ModifierId=Id; Modifier.PointCost=1; Modifier.RadiusDelta=10;
        }
        RegisterAbilityTestDefinition(Skills,Definition,false);
        return Definition;
    };
    auto* A=MakeDefinition(); auto* B=MakeDefinition(); auto* LearnedOnly=MakeDefinition();
    B->CooldownTag=FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.Winterbite"));
    LearnedOnly->CooldownTag=FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.ObsidianMaw"));
    TestEqual(TEXT("catalog visibility does not grant ownership"),Skills->Preflight(EMythicAbilityAction::Equip,0,A,NAME_None,false),EMythicAbilityResult::LockedAbility);
    Skills->ServerRequestAbilityAction(FGuid::NewGuid(),EMythicAbilityAction::Equip,0,A,NAME_None,false);
    TestNull(TEXT("a forged equip request cannot bypass the reward"),Skills->GetSkillInSlot(0));
    auto* PC=World->SpawnActor<APlayerController>(); PC->PlayerState=Player;
    auto* Reward=NewObject<UAbilityReward>(); Reward->SkillDefinition=A; FRewardContext Context(PC);
    TestTrue(TEXT("the common reward path learns the ability"),Reward->Give(Context));
    TestTrue(TEXT("reward replay is idempotent"),Reward->Give(Context));
    TestEqual(TEXT("reward replay stores one learned row"),Skills->GetSkillProgress().Num(),1);
    TestEqual(TEXT("learning does not grant an active skill spec"),ASC->GetActivatableAbilities().Num(),0);
    Skills->LearnSkill(B); Skills->LearnSkill(LearnedOnly); Skills->GrantSlot();
    Skills->ServerEquipSkill(0,A); Skills->ServerEquipSkill(1,B);
    TestTrue(TEXT("Q contains its selected ability"),Skills->GetSkillInSlot(0)==A);
    TestTrue(TEXT("E contains a different selected ability"),Skills->GetSkillInSlot(1)==B);
    Skills->GrantSlot(); TestEqual(TEXT("legacy slot rewards cannot exceed four sockets"),Skills->GetUnlockedSlots(),UMythicSkillComponent::SlotCount);
    TestEqual(TEXT("forged fifth-slot requests fail"),Skills->Preflight(EMythicAbilityAction::Equip,4,LearnedOnly,NAME_None,false),EMythicAbilityResult::InvalidSlot);
    TestEqual(TEXT("ownership permits a learned ability in R"),Skills->Preflight(EMythicAbilityAction::Equip,2,LearnedOnly,NAME_None,false),EMythicAbilityResult::Success);
    TestEqual(TEXT("the fourth slot cannot duplicate Q"),Skills->Preflight(EMythicAbilityAction::Equip,3,A,NAME_None,false),EMythicAbilityResult::EquippedElsewhere);
    if (!TestTrue(TEXT("the fixture defines stable saved transformation choices"),!A->Modifiers.IsEmpty())) return false;
    const FName ModifierId=A->Modifiers[0].ModifierId;
    Skills->ServerRequestAbilityAction(FGuid::NewGuid(),EMythicAbilityAction::Modifier,0,A,ModifierId,true);
    TestTrue(TEXT("the learned transformation can be selected"),Skills->IsModifierActive(A,0));
    TestTrue(TEXT("a successful cast can begin a durable cooldown"),Skills->StartCooldown(A,30));
    const float Before=Skills->GetCooldownRemaining(A);
    Skills->ServerUnequipSkill(0);
    TestTrue(TEXT("unequipping does not erase cooldown"),Skills->GetCooldownRemaining(A)>=Before-.1f);
    Skills->ServerEquipSkill(0,A);
    for (auto Handle:ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(A->CooldownTag.GetSingleTagContainer()))) ASC->RemoveActiveGameplayEffect(Handle);
    TestTrue(TEXT("dispel cannot erase the authority cooldown clock"),Skills->GetCooldownRemaining(A)>29);
    FSerializedCharacterData Saved;
    if (!TestTrue(TEXT("real character serialization succeeds"),FSerializedCharacterData::Serialize(Player,Saved))) return false;
    TestEqual(TEXT("all learned abilities are saved, including unequipped ones"),Saved.SkillProgress.Num(),3);
    TestEqual(TEXT("the live cooldown is part of the character save"),Saved.AbilityCooldowns.Num(),1);
    TestTrue(TEXT("cooldown shortening attempts cannot refresh the ability"),Skills->StartCooldown(A,1));
    TestTrue(TEXT("starting a shorter cooldown preserves the longer debt"),Skills->GetCooldownRemaining(A)>29);
    // Reordering authored rows must not change which behavior a saved choice means.
    if (A->Modifiers.Num()>1) A->Modifiers.Swap(0,1);
    Skills->ServerUnequipSkill(0); Skills->ServerUnequipSkill(1);
    if (!TestTrue(TEXT("real character deserialization succeeds"),FSerializedCharacterData::Deserialize(Player,Saved))) return false;
    TestTrue(TEXT("Q restores through the character integration"),Skills->GetSkillInSlot(0)==A);
    TestTrue(TEXT("E restores through the character integration"),Skills->GetSkillInSlot(1)==B);
    TestTrue(TEXT("unequipped ownership survives reload"),Skills->IsSkillUnlocked(LearnedOnly));
    TestTrue(TEXT("the active transformation follows its stable ID after reorder"),Skills->IsModifierActive(A,A->FindModifier(ModifierId)));
    TestTrue(TEXT("save/load pauses remaining cooldown without clearing it"),Skills->GetCooldownRemaining(A)>29);
    TestTrue(TEXT("restored state is ready for requests"),Skills->IsReady());
    TestTrue(TEXT("replaying the same character save is safe"),FSerializedCharacterData::Deserialize(Player,Saved));
    TestEqual(TEXT("load replay grants exactly Q and E"),ASC->GetActivatableAbilities().Num(),2);
    TestEqual(TEXT("load replay cannot duplicate ownership"),Skills->GetSkillProgress().Num(),3);
    FSerializedCharacterData RoundTrip;
    TestTrue(TEXT("the reconstructed character serializes again"),FSerializedCharacterData::Serialize(Player,RoundTrip));
    TestEqual(TEXT("cooldown debt survives the complete round trip"),RoundTrip.AbilityCooldowns.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityLifecycleTest,"Mythic.Progression.Skills.AbilityLifecycleEvents",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityLifecycleTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Avatar=InitializeAbilityTestAvatar(Player);
    if (!TestNotNull(TEXT("a real gameplay avatar owns the native life component"),Avatar)) return false;
    auto* Skills=Player->GetSkillComponent(); auto* ASC=Player->GetMythicAbilitySystemComponent();
    // Exercise production lifecycle without depending on a retired designer prototype's behavior.
    auto* Shipped=MakeAbilityWindowTestDefinition(GetTransientPackage(),FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.CinderLash")));
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetMaxStaminaAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),100);
    auto* Life=UMythicLifeComponent::FindHealthComponent(Avatar);
    if (!TestNotNull(TEXT("production actor initialization populates the cached life component"),Life)) return false;
    if (!TestTrue(TEXT("the real avatar life component is initialized"),Life->IsInitialized())) return false;
    if (!TestTrue(TEXT("the real avatar can pay stamina"),Life->CanSpendStamina(10))) return false;
    TestTrue(TEXT("GAS actor info points at the possessed avatar"),ASC->GetAvatarActor()==Avatar && Avatar->GetPlayerState()==Player);
    int32 Begins=0,Ends=0; FGuid BegunActivation;
    auto Begin=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_BEGIN).AddLambda([&](const FGameplayEventData* Event) {
        ++Begins;
        const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Event->ContextHandle);
        TestTrue(TEXT("skill lifecycle carries its source and activation identity"),Context && Context->DamageOrigin==EMythicDamageOrigin::Skill && Context->SourceActivationId.IsValid());
        if (Context) BegunActivation=Context->SourceActivationId;
        TestNull(TEXT("a solo cast cannot target its caster for Other talents"),Event->Target.Get());
    });
    auto End=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_END).AddLambda([&](const FGameplayEventData* Event) {
        ++Ends;
        const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Event->ContextHandle);
        TestTrue(TEXT("finish event belongs to the committed cast"),Context && Context->SourceActivationId==BegunActivation);
        TestNull(TEXT("solo finish does not debuff the caster"),Event->Target.Get());
    });
    for (int32 Attempt=0;Attempt<2;++Attempt) {
        auto* Definition=DuplicateObject<UMythicSkillDefinition>(Shipped,GetTransientPackage());
        if (Attempt) Definition->CooldownTag=FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ability.Winterbite"));
        RegisterAbilityTestDefinition(Skills,Definition); Skills->ServerEquipSkill(0,Definition);
        FGameplayAbilitySpec* Spec=nullptr;
        for (auto& Candidate:ASC->GetActivatableAbilities()) if (Candidate.SourceObject.Get()==Definition) { Spec=&Candidate; break; }
        if (!TestNotNull(TEXT("equipped ability grants one spec"),Spec)) break;
        auto* Runtime=Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance());
        if (!TestNotNull(TEXT("equipped runtime exists before activation"),Runtime)) break;
        FGameplayTagContainer FailureTags;
        TestTrue(TEXT("lifecycle fixture passes the production cost gate"),Runtime->CheckCost(Spec->Handle,ASC->AbilityActorInfo.Get(),&FailureTags));
        TestTrue(TEXT("lifecycle fixture passes the production cooldown gate"),Runtime->CheckCooldown(Spec->Handle,ASC->AbilityActorInfo.Get(),&FailureTags));
        TestTrue(TEXT("authority can activate the equipped ability"),ASC->TryActivateAbility(Spec->Handle));
        auto* Window=Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance());
        if (!TestNotNull(TEXT("the authored ability uses the window runtime"),Window)) break;
        TestTrue(TEXT("the authored ability committed"),Window->IsWindowAuthority());
        Window->EndAbilityWindow(Attempt ? EMythicAbilityWindowEnd::Cancelled:EMythicAbilityWindowEnd::Completed);
    }
    TestEqual(TEXT("each committed cast publishes one begin"),Begins,2);
    TestEqual(TEXT("cancellation does not publish a finish reward"),Ends,1);
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_BEGIN).Remove(Begin);
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_SKILL_END).Remove(End);
    return true;
}
#endif
