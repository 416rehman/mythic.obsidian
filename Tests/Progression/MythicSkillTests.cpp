
#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicAchievementComponent.h"
#include "Progression/MythicUnlockComponent.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "Subsystem/SaveSystem/MythicSaveGame.h"
#include "UObject/UnrealType.h"

namespace {

// Every skill verb is authority-gated and grants on the OWNER's ASC, so none of them can be reached without a live
// owner. The player state is that owner in the game and already carries both the ability system and the slots, so
// spawning one is the honest fixture rather than a stand-in.
struct FMythicSkillFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicSkillComponent *Skills = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
};

bool BuildSkillFixture(FAutomationTestBase &Test, FMythicSkillFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    Out.PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)) {
        return false;
    }
    if (!Test.TestTrue(TEXT("the owner holds authority"), Out.PlayerState->HasAuthority())) {
        return false;
    }
    Out.Skills = Out.PlayerState->GetSkillComponent();
    if (!Test.TestNotNull(TEXT("the player state owns a skill component"), Out.Skills)) {
        return false;
    }
    Out.ASC = Out.PlayerState->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns an ability system"), Out.ASC)) {
        return false;
    }
    if (!Out.Skills->IsRegistered()) {
        Out.Skills->RegisterComponent();
    }
    if (!Out.ASC->IsRegistered()) {
        Out.ASC->RegisterComponent();
    }
    Out.ASC->InitAbilityActorInfo(Out.PlayerState, Out.PlayerState);
    return Test.TestTrue(TEXT("the ability system answers as authority"), Out.ASC->IsOwnerActorAuthoritative());
}

UMythicSkillDefinition *MakeSkillDef(const FGameplayTag RequiredTag) {
    UMythicSkillDefinition *Skill = NewObject<UMythicSkillDefinition>();
    Skill->Ability = UGameplayAbility::StaticClass();
    Skill->RequiredTag = RequiredTag;
    return Skill;
}

int32 OpenEverySkillSlot(UMythicSkillComponent *Skills) {
    for (int32 Attempt = 0; Attempt < Skills->MaxSlots + 4; Attempt++) {
        Skills->GrantSlot();
    }
    return Skills->GetUnlockedSlots();
}

// Every public reader of the slot count clamps to MaxSlots, so a count that ran past the ceiling is invisible through
// the component's own API. Reflection is the only way to assert the stored number itself.
int32 RawUnlockedSkillSlots(const UMythicSkillComponent *Skills) {
    const FIntProperty *Field = CastField<FIntProperty>(
        UMythicSkillComponent::StaticClass()->FindPropertyByName(FName("UnlockedSlots")));
    return Field ? Field->GetPropertyValue_InContainer(Skills) : INDEX_NONE;
}

// A skill's ability is granted with the skill as the spec's source object, so the ASC's own list says whether it is
// still bound. The component keeps its handles private; this is the seam it leaves open.
const FGameplayAbilitySpec *FindGrantedSkillSpec(const UAbilitySystemComponent *ASC, const UMythicSkillDefinition *Skill) {
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove && Spec.SourceObject.Get() == Skill) {
            return &Spec;
        }
    }
    return nullptr;
}

int32 CountGrantedSkillSpecs(const UAbilitySystemComponent *ASC, const UMythicSkillDefinition *Skill) {
    int32 Count = 0;
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove && Spec.SourceObject.Get() == Skill) {
            Count++;
        }
    }
    return Count;
}

int32 CountLiveSkillSpecs(const UAbilitySystemComponent *ASC) {
    int32 Count = 0;
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove) {
            Count++;
        }
    }
    return Count;
}

FGameplayTag SkillTestTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillSlotGatingTest,
    "Mythic.Progression.Skills.SlotGating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSlotGatingTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room to grow"), Skills->MaxSlots >= 2)) {
        return false;
    }

    // A fresh character carries one skill and no more. Slot two upward is a deed nobody has done yet.
    TestEqual(TEXT("a fresh character starts with one slot"), Skills->GetUnlockedSlots(), 1);
    TestTrue(TEXT("slot one is open"), Skills->IsSlotUnlocked(0));
    for (int32 Slot = 1; Slot < Skills->MaxSlots; Slot++) {
        TestFalse(FString::Printf(TEXT("slot %d is shut until it is earned"), Slot), Skills->IsSlotUnlocked(Slot));
    }
    TestFalse(TEXT("a negative slot is never open"), Skills->IsSlotUnlocked(-1));

    Skills->GrantSlot();
    TestEqual(TEXT("one unlock opens exactly one more slot"), Skills->GetUnlockedSlots(), 2);
    TestTrue(TEXT("the newly granted slot is open"), Skills->IsSlotUnlocked(1));
    if (Skills->MaxSlots > 2) {
        TestFalse(TEXT("the slot after it is still shut"), Skills->IsSlotUnlocked(2));
    }

    // Rules fire more than once in a long save. The count must stop at the authored ceiling, not run away.
    const int32 Ceiling = Skills->MaxSlots;
    OpenEverySkillSlot(Skills);
    TestEqual(TEXT("repeated grants stop at MaxSlots"), RawUnlockedSkillSlots(Skills), Ceiling);
    TestTrue(TEXT("the last slot is open"), Skills->IsSlotUnlocked(Ceiling - 1));
    TestFalse(TEXT("nothing opens past MaxSlots"), Skills->IsSlotUnlocked(Ceiling));

    // A count that overshot stays hidden behind the clamp until the ceiling moves, then pays out slots nobody earned.
    // Raising MaxSlots is what makes the stored number answer for itself.
    Skills->MaxSlots = Ceiling + 2;
    TestEqual(TEXT("a raised ceiling opens no slot the player did not earn"), Skills->GetUnlockedSlots(), Ceiling);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillLockedSlotRefusedTest,
    "Mythic.Progression.Skills.LockedSlotRefused",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillLockedSlotRefusedTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the authored MaxSlots leaves a shut slot in range"), Skills->MaxSlots >= 2)) {
        return false;
    }
    if (!TestEqual(TEXT("a fresh character starts with one slot"), Skills->GetUnlockedSlots(), 1)) {
        return false;
    }

    UMythicSkillDefinition *First = MakeSkillDef(FGameplayTag());
    UMythicSkillDefinition *Second = MakeSkillDef(FGameplayTag());

    // Slot two is a real entry in the bar, just an unearned one. The range check cannot be what refuses it.
    Skills->ServerEquipSkill(1, Second);
    if (!TestTrue(TEXT("slot two is a real index, so only the lock can refuse it"),
                  Skills->GetEquippedSkills().IsValidIndex(1))) {
        return false;
    }
    TestNull(TEXT("a shut slot takes no skill"), Skills->GetSkillInSlot(1));
    TestEqual(TEXT("and grants nothing"), CountGrantedSkillSpecs(Fixture.ASC, Second), 0);

    // The denominator: the open slot takes a skill, and slot two takes one the moment it is earned.
    Skills->ServerEquipSkill(0, First);
    TestTrue(TEXT("the open slot takes its skill"), Skills->GetSkillInSlot(0) == First);

    Skills->GrantSlot();
    Skills->ServerEquipSkill(1, Second);
    TestTrue(TEXT("the earned slot takes the skill it just refused"), Skills->GetSkillInSlot(1) == Second);
    TestEqual(TEXT("and grants its ability"), CountGrantedSkillSpecs(Fixture.ASC, Second), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillSlotRangeTest,
    "Mythic.Progression.Skills.SlotRange",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSlotRangeTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    OpenEverySkillSlot(Skills);

    UMythicSkillDefinition *Bound = MakeSkillDef(FGameplayTag());
    Skills->ServerEquipSkill(0, Bound);
    const int32 Slots = Skills->GetEquippedSkills().Num();
    if (!TestEqual(TEXT("the slot bar is MaxSlots long"), Slots, Skills->MaxSlots)) {
        return false;
    }

    // A client can name any index it likes. None of these may grow the bar or reach the array.
    Skills->ServerEquipSkill(-1, Bound);
    Skills->ServerEquipSkill(Slots, Bound);
    Skills->ServerEquipSkill(TNumericLimits<int32>::Max(), Bound);
    Skills->ServerUnequipSkill(-1);
    Skills->ServerUnequipSkill(Slots);
    Skills->ServerUnequipSkill(TNumericLimits<int32>::Max());

    TestEqual(TEXT("an out-of-range index never grows the bar"), Skills->GetEquippedSkills().Num(), Slots);
    TestNull(TEXT("nothing landed off the end"), Skills->GetSkillInSlot(Slots));
    TestNull(TEXT("nor before the start"), Skills->GetSkillInSlot(-1));
    TestTrue(TEXT("and the bound skill is untouched"), Skills->GetSkillInSlot(0) == Bound);
    TestEqual(TEXT("granted exactly once throughout"), CountGrantedSkillSpecs(Fixture.ASC, Bound), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillDeedGatingTest,
    "Mythic.Progression.Skills.DeedGating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillDeedGatingTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = SkillTestTag(TEXT("Achievement.Delver"));
    const FGameplayTag UnearnedDeed = SkillTestTag(TEXT("Achievement.Wanderer"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Delver / Achievement.Wanderer are not registered in this build; deed gating untested"));
        return true;
    }

    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;

    // #46 says the deed may be recorded anywhere, so the ASC's own tags count as proof.
    Fixture.ASC->AddLooseGameplayTag(EarnedDeed);

    UMythicSkillDefinition *Deeded = MakeSkillDef(EarnedDeed);
    UMythicSkillDefinition *Locked = MakeSkillDef(UnearnedDeed);
    UMythicSkillDefinition *Free = MakeSkillDef(FGameplayTag());

    TestTrue(TEXT("a skill whose deed is done is available"), Skills->IsSkillUnlocked(Deeded));
    TestFalse(TEXT("a skill whose deed is undone is not"), Skills->IsSkillUnlocked(Locked));
    TestTrue(TEXT("a skill with no required tag needs no deed"), Skills->IsSkillUnlocked(Free));
    TestFalse(TEXT("a null skill is never unlocked"), Skills->IsSkillUnlocked(nullptr));

    // The predicate is advice; the verb is the gate. Both have to hold.
    Skills->ServerEquipSkill(0, Locked);
    TestNull(TEXT("an unearned skill never reaches a slot"), Skills->GetSkillInSlot(0));
    TestEqual(TEXT("and nothing is granted for it"), CountGrantedSkillSpecs(Fixture.ASC, Locked), 0);

    Skills->ServerEquipSkill(0, Free);
    TestTrue(TEXT("a skill needing no deed equips"), Skills->GetSkillInSlot(0) == Free);

    Skills->ServerEquipSkill(0, Deeded);
    TestTrue(TEXT("an earned skill equips"), Skills->GetSkillInSlot(0) == Deeded);
    TestEqual(TEXT("granting its ability"), CountGrantedSkillSpecs(Fixture.ASC, Deeded), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillAchievementDeedTest,
    "Mythic.Progression.Skills.AchievementDeed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillAchievementDeedTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = SkillTestTag(TEXT("Achievement.Smith"));
    const FGameplayTag UnearnedDeed = SkillTestTag(TEXT("Achievement.Forager"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Smith / Achievement.Forager are not registered in this build; achievement gating untested"));
        return true;
    }

    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;

    UMythicAchievementComponent *Achievements = Fixture.PlayerState->GetAchievementComponent();
    if (!TestNotNull(TEXT("the player state owns an achievement ledger"), Achievements)) {
        return false;
    }
    FGameplayTagContainer Deeds;
    Deeds.AddTag(EarnedDeed);
    Achievements->RestoreUnlockedAchievements(Deeds);
    if (!TestTrue(TEXT("the ledger holds the deed"), Achievements->IsAchievementUnlocked(EarnedDeed))) {
        return false;
    }

    // The achievement ledger is the only place this deed is written. If the skill gate cannot read that ledger it has
    // nowhere else to find the tag, so every Achievement.* skill stays locked forever — DA_Unlock_SkillSlot2's own
    // gate is Achievement.BossHunter, so this is the live case, not a hypothetical one.
    TestFalse(TEXT("the ability system does not hold the deed"), Fixture.ASC->HasMatchingGameplayTag(EarnedDeed));
    if (const UMythicUnlockComponent *Unlocks = Fixture.PlayerState->GetUnlockComponent()) {
        TestFalse(TEXT("nor does the unlock ledger"), Unlocks->HasUnlockTag(EarnedDeed));
    }

    UMythicSkillDefinition *Deeded = MakeSkillDef(EarnedDeed);
    UMythicSkillDefinition *Locked = MakeSkillDef(UnearnedDeed);

    TestTrue(TEXT("a skill earned by an achievement is available"), Skills->IsSkillUnlocked(Deeded));
    TestFalse(TEXT("a skill whose achievement is undone is not"), Skills->IsSkillUnlocked(Locked));

    Skills->ServerEquipSkill(0, Deeded);
    TestTrue(TEXT("and the verb lets it into a slot"), Skills->GetSkillInSlot(0) == Deeded);
    TestEqual(TEXT("granting its ability"), CountGrantedSkillSpecs(Fixture.ASC, Deeded), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillPayloadRequiredTest,
    "Mythic.Progression.Skills.PayloadRequired",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillPayloadRequiredTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    const int32 SpecsBefore = CountLiveSkillSpecs(Fixture.ASC);

    UMythicSkillDefinition *Inert = NewObject<UMythicSkillDefinition>();
    TestFalse(TEXT("a skill with no ability has no payload"), Inert->HasPayload());

    Skills->ServerEquipSkill(0, Inert);
    TestNull(TEXT("an inert skill never occupies a slot"), Skills->GetSkillInSlot(0));

    Skills->ServerEquipSkill(0, nullptr);
    TestNull(TEXT("nor does nothing at all"), Skills->GetSkillInSlot(0));

    // The refusal has to happen before the ASC is asked. GiveAbility would reject a null class anyway, but it does so
    // by logging an error, which is a shipped-game log spike rather than a clean refusal.
    TestEqual(TEXT("the ability system was never asked"), CountLiveSkillSpecs(Fixture.ASC), SpecsBefore);

    // The denominator: the same slot, same call, a skill that does carry an ability.
    UMythicSkillDefinition *Carrying = MakeSkillDef(FGameplayTag());
    TestTrue(TEXT("a skill with an ability has a payload"), Carrying->HasPayload());
    Skills->ServerEquipSkill(0, Carrying);
    TestTrue(TEXT("and it equips"), Skills->GetSkillInSlot(0) == Carrying);
    TestEqual(TEXT("granting exactly one spec"), CountLiveSkillSpecs(Fixture.ASC), SpecsBefore + 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillOneSlotEachTest,
    "Mythic.Progression.Skills.OneSlotEach",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillOneSlotEachTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the character can open at least two slots"), OpenEverySkillSlot(Skills) >= 2)) {
        return false;
    }

    UMythicSkillDefinition *Bound = MakeSkillDef(FGameplayTag());
    Skills->ServerEquipSkill(0, Bound);
    if (!TestTrue(TEXT("the skill took the first slot"), Skills->GetSkillInSlot(0) == Bound)) {
        return false;
    }

    // The same skill on both keys would be two cooldowns for one skill. It must be refused, and the original binding
    // must survive the refusal.
    Skills->ServerEquipSkill(1, Bound);
    TestNull(TEXT("the same skill cannot fill both slots"), Skills->GetSkillInSlot(1));
    TestTrue(TEXT("the first binding survives the refusal"), Skills->GetSkillInSlot(0) == Bound);
    TestEqual(TEXT("and it is still granted exactly once"), CountGrantedSkillSpecs(Fixture.ASC, Bound), 1);

    // The denominator: a different skill still fits that slot.
    UMythicSkillDefinition *Other = MakeSkillDef(FGameplayTag());
    Skills->ServerEquipSkill(1, Other);
    TestTrue(TEXT("a different skill fits the second slot"), Skills->GetSkillInSlot(1) == Other);
    TestTrue(TEXT("and the first slot is still its own"), Skills->GetSkillInSlot(0) == Bound);

    // Moving a skill is not the same as doubling it: the first slot must be emptied first.
    Skills->ServerUnequipSkill(0);
    Skills->ServerEquipSkill(1, Bound);
    TestTrue(TEXT("a freed skill moves to the other slot"), Skills->GetSkillInSlot(1) == Bound);
    TestEqual(TEXT("still granted exactly once after the move"), CountGrantedSkillSpecs(Fixture.ASC, Bound), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillAbilityLifetimeTest,
    "Mythic.Progression.Skills.AbilityLifetime",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillAbilityLifetimeTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the character can open at least one slot"), OpenEverySkillSlot(Skills) >= 1)) {
        return false;
    }

    UMythicSkillDefinition *First = MakeSkillDef(FGameplayTag());
    UMythicSkillDefinition *Second = MakeSkillDef(FGameplayTag());

    Skills->ServerEquipSkill(0, First);
    if (!TestEqual(TEXT("equipping a skill grants its ability once"), CountGrantedSkillSpecs(Fixture.ASC, First), 1)) {
        return false;
    }

    // Swapping a slot is where a leak pays: the old ability keeps answering the key for a skill the player unslotted.
    Skills->ServerEquipSkill(0, Second);
    TestTrue(TEXT("the slot now holds the second skill"), Skills->GetSkillInSlot(0) == Second);
    TestEqual(TEXT("the replaced skill's ability is handed back"), CountGrantedSkillSpecs(Fixture.ASC, First), 0);
    TestEqual(TEXT("and the new one is granted once"), CountGrantedSkillSpecs(Fixture.ASC, Second), 1);

    // Unslotting must clear the ability, not merely blank the array entry.
    Skills->ServerUnequipSkill(0);
    TestNull(TEXT("the slot is empty"), Skills->GetSkillInSlot(0));
    TestEqual(TEXT("and its ability is gone"), CountGrantedSkillSpecs(Fixture.ASC, Second), 0);

    // The denominator: equipping it again grants it back, exactly once.
    Skills->ServerEquipSkill(0, Second);
    TestEqual(TEXT("re-equipping grants it once more"), CountGrantedSkillSpecs(Fixture.ASC, Second), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillInputBindingTest,
    "Mythic.Progression.Skills.InputBinding",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillInputBindingTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the character can open at least two slots"), OpenEverySkillSlot(Skills) >= 2)) {
        return false;
    }

    // The slot IS the key binding. AbilityInputTagPressed matches a pressed key against the granted spec's dynamic
    // source tags, so a spec without its slot's tag is a skill no key can ever fire.
    const FGameplayTag SlotZeroKey = Skills->GetSlotInputTag(0);
    const FGameplayTag SlotOneKey = Skills->GetSlotInputTag(1);
    if (!TestTrue(TEXT("slot one answers to a key"), SlotZeroKey.IsValid())) {
        return false;
    }
    if (!TestTrue(TEXT("slot two answers to a key"), SlotOneKey.IsValid())) {
        return false;
    }
    TestNotEqual(TEXT("the two slots answer to different keys"), SlotZeroKey, SlotOneKey);

    // The authored defaults are Q and E, per #46. If they ever go missing the binding still has to hold, so this is
    // a note rather than a gate.
    const FGameplayTag QKey = SkillTestTag(TEXT("Input.Action.SkillQ"));
    const FGameplayTag EKey = SkillTestTag(TEXT("Input.Action.SkillE"));
    if (QKey.IsValid() && EKey.IsValid()) {
        TestEqual(TEXT("slot one is authored as Q"), SlotZeroKey, QKey);
        TestEqual(TEXT("slot two is authored as E"), SlotOneKey, EKey);
    }
    else {
        AddWarning(TEXT("Input.Action.SkillQ / SkillE are not registered in this build; the authored defaults are untested"));
    }

    UMythicSkillDefinition *OnQ = MakeSkillDef(FGameplayTag());
    UMythicSkillDefinition *OnE = MakeSkillDef(FGameplayTag());
    Skills->ServerEquipSkill(0, OnQ);
    Skills->ServerEquipSkill(1, OnE);

    const FGameplayAbilitySpec *QSpec = FindGrantedSkillSpec(Fixture.ASC, OnQ);
    const FGameplayAbilitySpec *ESpec = FindGrantedSkillSpec(Fixture.ASC, OnE);
    if (!TestNotNull(TEXT("the first slot granted a spec"), QSpec) || !TestNotNull(TEXT("the second slot granted a spec"), ESpec)) {
        return false;
    }

    TestTrue(TEXT("the first slot's skill answers the first slot's key"),
             QSpec->GetDynamicSpecSourceTags().HasTagExact(SlotZeroKey));
    TestFalse(TEXT("and does not answer the other key"), QSpec->GetDynamicSpecSourceTags().HasTagExact(SlotOneKey));
    TestTrue(TEXT("the second slot's skill answers the second slot's key"),
             ESpec->GetDynamicSpecSourceTags().HasTagExact(SlotOneKey));
    TestFalse(TEXT("and does not answer the other key"), ESpec->GetDynamicSpecSourceTags().HasTagExact(SlotZeroKey));

    // The binding belongs to the slot, not to the skill. Moving a skill has to move which key fires it.
    Skills->ServerUnequipSkill(0);
    Skills->ServerUnequipSkill(1);
    Skills->ServerEquipSkill(1, OnQ);
    const FGameplayAbilitySpec *MovedSpec = FindGrantedSkillSpec(Fixture.ASC, OnQ);
    if (!TestNotNull(TEXT("the moved skill is granted again"), MovedSpec)) {
        return false;
    }
    TestTrue(TEXT("the moved skill now answers the second slot's key"),
             MovedSpec->GetDynamicSpecSourceTags().HasTagExact(SlotOneKey));
    TestFalse(TEXT("and no longer answers the first"), MovedSpec->GetDynamicSpecSourceTags().HasTagExact(SlotZeroKey));

    // The key per slot is authored data. Rewriting the row must rewrite the binding, or a third slot would need code.
    const FGameplayTag Rebound = SkillTestTag(TEXT("Input.Action.Ability1"));
    if (Rebound.IsValid() && Rebound != SlotOneKey && Skills->SlotInputTags.IsValidIndex(1)) {
        Skills->SlotInputTags[1] = Rebound;
        Skills->ServerUnequipSkill(1);
        Skills->ServerEquipSkill(1, OnQ);
        const FGameplayAbilitySpec *ReboundSpec = FindGrantedSkillSpec(Fixture.ASC, OnQ);
        if (TestNotNull(TEXT("the rebound slot granted a spec"), ReboundSpec)) {
            TestTrue(TEXT("the slot's key comes from the authored row, not a constant"),
                     ReboundSpec->GetDynamicSpecSourceTags().HasTagExact(Rebound));
            TestFalse(TEXT("the old key no longer fires it"), ReboundSpec->GetDynamicSpecSourceTags().HasTagExact(SlotOneKey));
        }
    }
    else {
        AddWarning(TEXT("Input.Action.Ability1 is not registered in this build; the authored-row rebind is untested"));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillSaveFieldsTest,
    "Mythic.Progression.Skills.SaveFields",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSaveFieldsTest::RunTest(const FString &Parameters) {
    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the character can open at least two slots"), OpenEverySkillSlot(Skills) >= 2)) {
        return false;
    }

    UMythicSkillDefinition *Bound = MakeSkillDef(FGameplayTag());
    Skills->ServerEquipSkill(1, Bound);
    if (!TestTrue(TEXT("the skill is bound"), Skills->GetSkillInSlot(1) == Bound)) {
        return false;
    }

    // The component's SaveGame flag is inert here: this project persists an explicit field list, so slots and skills
    // only survive a reload if the character save writes them itself.
    FSerializedCharacterData Data;
    if (!TestTrue(TEXT("the character serialises"), FSerializedCharacterData::Serialize(Fixture.PlayerState, Data))) {
        return false;
    }

    TestEqual(TEXT("the save carries the open slot count"), Data.UnlockedSkillSlots, Skills->GetUnlockedSlots());
    if (!TestEqual(TEXT("the save carries one entry per slot"), Data.EquippedSkills.Num(),
                   Skills->GetEquippedSkills().Num())) {
        return false;
    }
    TestEqual(TEXT("the bound skill is written at its own slot"), Data.EquippedSkills[1].ToString(),
              FSoftObjectPath(Bound).ToString());
    TestTrue(TEXT("an empty slot is written as an empty path"), Data.EquippedSkills[0].IsNull());
    TestEqual(TEXT("the save is stamped at the current version"), Data.DataVersion,
              static_cast<int32>(CurrentCharacterSaveVersion));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillRestoreTest,
    "Mythic.Progression.Skills.Restore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillRestoreTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = SkillTestTag(TEXT("Achievement.Smith"));
    const FGameplayTag UnearnedDeed = SkillTestTag(TEXT("Achievement.Forager"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Smith / Achievement.Forager are not registered in this build; skill restore untested"));
        return true;
    }

    FMythicSkillFixture Fixture;
    const bool bReady = BuildSkillFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    const int32 Ceiling = Skills->MaxSlots;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room for a dropped skill"), Ceiling >= 2)) {
        return false;
    }

    UMythicAchievementComponent *Achievements = Fixture.PlayerState->GetAchievementComponent();
    if (!TestNotNull(TEXT("the player state owns an achievement ledger"), Achievements)) {
        return false;
    }
    FGameplayTagContainer Deeds;
    Deeds.AddTag(EarnedDeed);
    Achievements->RestoreUnlockedAchievements(Deeds);

    UMythicSkillDefinition *Deeded = MakeSkillDef(EarnedDeed);
    UMythicSkillDefinition *Lost = MakeSkillDef(UnearnedDeed);

    // The save as it comes off disk: a bound skill, a skill whose deed this character no longer has, and a slot count
    // from a character who was later given a smaller ceiling.
    TArray<TSoftObjectPtr<UMythicSkillDefinition>> Saved;
    Saved.Add(Deeded);
    Saved.Add(Lost);
    if (!TestFalse(TEXT("a saved skill carries a real path"), Saved[0].ToSoftObjectPath().IsNull())) {
        return false;
    }

    Skills->RestoreSkills(Saved, Ceiling + 5);

    TestEqual(TEXT("a saved count above the ceiling is clamped to it"), RawUnlockedSkillSlots(Skills), Ceiling);
    TestEqual(TEXT("the slot bar is sized to the ceiling"), Skills->GetEquippedSkills().Num(), Ceiling);
    TestTrue(TEXT("the bound skill comes back to its slot"), Skills->GetSkillInSlot(0) == Deeded);
    TestEqual(TEXT("and its ability is granted again"), CountGrantedSkillSpecs(Fixture.ASC, Deeded), 1);
    TestNull(TEXT("a skill whose deed is gone is dropped"), Skills->GetSkillInSlot(1));
    TestEqual(TEXT("and nothing is granted for it"), CountGrantedSkillSpecs(Fixture.ASC, Lost), 0);

    // Restoring a slot without re-granting would give the player a filled bar whose keys do nothing, which is the
    // failure a bare array restore cannot see.
    const FGameplayAbilitySpec *RestoredSpec = FindGrantedSkillSpec(Fixture.ASC, Deeded);
    if (TestNotNull(TEXT("the restored skill has a live spec"), RestoredSpec)) {
        const FGameplayTag SlotZeroKey = Skills->GetSlotInputTag(0);
        if (SlotZeroKey.IsValid()) {
            TestTrue(TEXT("and it answers its slot's key after a reload"),
                     RestoredSpec->GetDynamicSpecSourceTags().HasTagExact(SlotZeroKey));
        }
    }

    // Loading twice in one session must not leave two copies of the same ability on the ASC, and an empty saved entry
    // must stay empty rather than sliding a skill up the bar.
    TArray<TSoftObjectPtr<UMythicSkillDefinition>> Reloaded;
    Reloaded.AddDefaulted();
    Reloaded.Add(Deeded);
    Skills->RestoreSkills(Reloaded, 2);

    TestEqual(TEXT("a saved count below the ceiling is kept, not reset to one"), RawUnlockedSkillSlots(Skills), 2);
    TestNull(TEXT("an empty slot stays empty"), Skills->GetSkillInSlot(0));
    TestTrue(TEXT("the skill is bound at the slot it was saved in"), Skills->GetSkillInSlot(1) == Deeded);
    TestEqual(TEXT("its ability is still granted exactly once"), CountGrantedSkillSpecs(Fixture.ASC, Deeded), 1);

    // The restored count is what the player grows from; a later unlock adds to it.
    Skills->MaxSlots = Ceiling + 1;
    Skills->GrantSlot();
    TestEqual(TEXT("a later unlock builds on the restored count"), Skills->GetUnlockedSlots(), 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillSlotCountMigrationTest,
    "Mythic.Progression.Skills.SlotCountMigration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSlotCountMigrationTest::RunTest(const FString &Parameters) {
    /**
     * A save written before skill slots were persisted carries the GrantSkillSlot rules but no count, and
     * RestoreUnlockState re-latches those rules so they never fire again. Without this repair a character who earned
     * the E slot reloads with only Q, permanently.
     */
    const FGameplayTag SkillSlot2 = SkillTestTag(TEXT("Unlock.Rule.SkillSlot2"));
    const FGameplayTag RuneSlot2 = SkillTestTag(TEXT("Unlock.Rule.RuneSlot2"));
    const FGameplayTag Unrelated = SkillTestTag(TEXT("Unlock.Rule.TitleBossHunter"));
    if (!SkillSlot2.IsValid()) {
        AddInfo(TEXT("Unlock.Rule.SkillSlot2 is not registered; skipping"));
        return true;
    }

    using Migration = FMythicCharacterSaveMigration;

    TestEqual(TEXT("a save that earned nothing still owns the free slot"),
              Migration::SkillSlotsFromAppliedRules({}), 1);
    TestEqual(TEXT("one earned rule reopens one slot"),
              Migration::SkillSlotsFromAppliedRules({SkillSlot2}), 2);

    // The scan must key on the skill rules, not simply count what was applied — and the rune rules sit under the same
    // Unlock.Rule parent, so a loose prefix would hand out a skill slot for every rune socket.
    if (RuneSlot2.IsValid()) {
        TestEqual(TEXT("a rune socket rule does not open a skill slot"),
                  Migration::SkillSlotsFromAppliedRules({RuneSlot2}), 1);
        TestEqual(TEXT("and does not inflate a genuine count"),
                  Migration::SkillSlotsFromAppliedRules({RuneSlot2, SkillSlot2, RuneSlot2}), 2);
    }
    if (Unrelated.IsValid()) {
        TestEqual(TEXT("an unrelated unlock does not open a slot"),
                  Migration::SkillSlotsFromAppliedRules({Unrelated}), 1);
    }

    TestEqual(TEXT("an invalid entry is ignored rather than counted"),
              Migration::SkillSlotsFromAppliedRules({FGameplayTag(), SkillSlot2}), 2);

    // The rune scan must stay symmetrical: a skill rule is not a rune socket either.
    TestEqual(TEXT("a skill slot rule does not open a rune socket"),
              Migration::RuneSlotsFromAppliedRules({SkillSlot2}), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillSaveFixupTest,
    "Mythic.Progression.Skills.SaveFixup",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSaveFixupTest::RunTest(const FString &Parameters) {
    const FGameplayTag SkillSlot2 = SkillTestTag(TEXT("Unlock.Rule.SkillSlot2"));
    const FGameplayTag RuneSlot2 = SkillTestTag(TEXT("Unlock.Rule.RuneSlot2"));
    if (!SkillSlot2.IsValid()) {
        AddInfo(TEXT("Unlock.Rule.SkillSlot2 is not registered; skipping"));
        return true;
    }

    // A rune-era save: it earned the E slot before GrantSkillSlot did anything, so it carries the rule and no count.
    UMythicSaveGame *RuneEra = NewObject<UMythicSaveGame>();
    RuneEra->CharacterData.DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::PreSkills);
    RuneEra->CharacterData.AppliedUnlockRules = {SkillSlot2};
    RuneEra->CharacterData.UnlockedSkillSlots = 0;
    RuneEra->FixupData();

    TestEqual(TEXT("the earned slot is rebuilt from the applied rule"), RuneEra->CharacterData.UnlockedSkillSlots, 2);
    TestEqual(TEXT("and the save is stamped forward"), RuneEra->CharacterData.DataVersion,
              static_cast<int32>(CurrentCharacterSaveVersion));

    // A save from before either system: both repairs have to run, not just the newest one.
    if (RuneSlot2.IsValid()) {
        UMythicSaveGame *Ancient = NewObject<UMythicSaveGame>();
        Ancient->CharacterData.DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::PreRunes);
        Ancient->CharacterData.AppliedUnlockRules = {RuneSlot2, SkillSlot2};
        Ancient->FixupData();
        TestEqual(TEXT("an older save still recovers its rune sockets"), Ancient->CharacterData.UnlockedRuneSlots, 2);
        TestEqual(TEXT("and its skill slots in the same pass"), Ancient->CharacterData.UnlockedSkillSlots, 2);
    }

    // The version marker has to be read, not merely written. A current save that spent its second slot back down to
    // one must keep that one, or every load would re-derive a count the player no longer has.
    UMythicSaveGame *Current = NewObject<UMythicSaveGame>();
    Current->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    Current->CharacterData.AppliedUnlockRules = {SkillSlot2};
    Current->CharacterData.UnlockedSkillSlots = 1;
    Current->FixupData();
    TestEqual(TEXT("a save already at the current version is left alone"), Current->CharacterData.UnlockedSkillSlots, 1);

    // A brand-new save writes no count, and the restore seam is what treats that as "leave the component alone".
    UMythicSaveGame *Fresh = NewObject<UMythicSaveGame>();
    Fresh->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    Fresh->FixupData();
    TestEqual(TEXT("a fresh save carries no slot count for the restore to apply"),
              Fresh->CharacterData.UnlockedSkillSlots, 0);

    return true;
}
