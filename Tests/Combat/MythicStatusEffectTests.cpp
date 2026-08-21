
#include "Misc/AutomationTest.h"

#include "GAS/Effects/MythicStatusEffects.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicTags_GAS.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusEffectTest,
    "Mythic.Combat.StatusEffects",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusEffectTest::RunTest(const FString &Parameters) {
    using Def = UMythicAttributeSet_Defense;

    const FGameplayTag TypeBurn = FGameplayTag::RequestGameplayTag(FName("Status.Type.Burn"));
    const FGameplayTag TypePoison = FGameplayTag::RequestGameplayTag(FName("Status.Type.Poison"));
    const FGameplayTag TypeBleed = FGameplayTag::RequestGameplayTag(FName("Status.Type.Bleed"));
    const FGameplayTag TypeSlow = FGameplayTag::RequestGameplayTag(FName("Status.Type.Slow"));
    const FGameplayTag TypeFreeze = FGameplayTag::RequestGameplayTag(FName("Status.Type.Freeze"));
    const FGameplayTag TypeStun = FGameplayTag::RequestGameplayTag(FName("Status.Type.Stun"));

    TestTrue(TEXT("resolve Burn"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypeBurn).Get() == UMythicGE_Burn::StaticClass());
    TestTrue(TEXT("resolve Poison"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypePoison).Get() == UMythicGE_Poison::StaticClass());
    TestTrue(TEXT("resolve Bleed"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypeBleed).Get() == UMythicGE_Bleed::StaticClass());
    TestTrue(TEXT("resolve Slow"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypeSlow).Get() == UMythicGE_Slow::StaticClass());
    TestTrue(TEXT("resolve Freeze"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypeFreeze).Get() == UMythicGE_Freeze::StaticClass());
    TestTrue(TEXT("resolve Stun"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(TypeStun).Get() == UMythicGE_Stun::StaticClass());

    const FGameplayTag Unrelated = FGameplayTag::RequestGameplayTag(FName("GAS.Debuff.Bleeding"));
    TestNull(TEXT("resolve unrelated tag → nullptr"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(Unrelated).Get());
    TestNull(TEXT("resolve invalid tag → nullptr"), FMythicStatusEffectResolver::ResolveDebuffGEForStatus(FGameplayTag()).Get());

    auto CheckGranted = [&](const UGameplayEffect *GE, const FGameplayTag &Expected, const TCHAR *Label) {
        const UTargetTagsGameplayEffectComponent *TagComp = GE ? GE->FindComponent<UTargetTagsGameplayEffectComponent>() : nullptr;
        if (TestNotNull(Label, TagComp)) {
            TestTrue(Label, TagComp->GetConfiguredTargetTagChanges().CombinedTags.HasTagExact(Expected));
        }
    };

    auto CheckDoT = [&](const UGameplayEffect *GE, const TCHAR *Label) {
        TestTrue(Label, GE->DurationPolicy == EGameplayEffectDurationType::HasDuration);
        TestTrue(Label, GE->Period.GetValueAtLevel(0.0f) > 0.0f);
        TestFalse(Label, GE->bExecutePeriodicEffectOnApplication);
        if (TestEqual(Label, GE->Modifiers.Num(), 1)) {
            TestTrue(Label, GE->Modifiers[0].ModifierOp == EGameplayModOp::Additive);
            TestTrue(Label, GE->Modifiers[0].Attribute == UMythicAttributeSet_Life::GetDamageAttribute());
        }
    };

    auto CheckTagOnly = [&](const UGameplayEffect *GE, const TCHAR *Label) {
        TestTrue(Label, GE->DurationPolicy == EGameplayEffectDurationType::HasDuration);
        TestEqual(Label, GE->Modifiers.Num(), 0);
    };

    const UMythicGE_Burn *Burn = GetDefault<UMythicGE_Burn>();
    const UMythicGE_Poison *Poison = GetDefault<UMythicGE_Poison>();
    const UMythicGE_Bleed *Bleed = GetDefault<UMythicGE_Bleed>();
    const UMythicGE_Slow *Slow = GetDefault<UMythicGE_Slow>();
    const UMythicGE_Freeze *Freeze = GetDefault<UMythicGE_Freeze>();
    const UMythicGE_Stun *Stun = GetDefault<UMythicGE_Stun>();
    const UMythicGE_Weaken *Weaken = GetDefault<UMythicGE_Weaken>();
    const UMythicGE_Terrify *Terrify = GetDefault<UMythicGE_Terrify>();

    CheckDoT(Burn, TEXT("Burn is a configured DoT"));
    CheckDoT(Poison, TEXT("Poison is a configured DoT"));
    CheckDoT(Bleed, TEXT("Bleed is a configured DoT"));
    CheckGranted(Burn, GAS_DEBUFF_BURNING, TEXT("Burn grants GAS.Debuff.Burning"));
    CheckGranted(Bleed, GAS_DEBUFF_BLEEDING, TEXT("Bleed grants GAS.Debuff.Bleeding"));
    CheckGranted(Poison, FGameplayTag::RequestGameplayTag(FName("Status.State.Poisoned")), TEXT("Poison grants Status.State.Poisoned"));
    CheckGranted(Poison, GAS_DEBUFF_POISONED, TEXT("Poison grants GAS.Debuff.Poisoned"));

    CheckTagOnly(Slow, TEXT("Slow is tag-only"));
    CheckTagOnly(Freeze, TEXT("Freeze is tag-only"));
    CheckTagOnly(Stun, TEXT("Stun is tag-only"));
    CheckTagOnly(Weaken, TEXT("Weaken is tag-only"));
    CheckTagOnly(Terrify, TEXT("Terrify is tag-only"));
    CheckGranted(Slow, GAS_DEBUFF_SLOWED, TEXT("Slow grants GAS.Debuff.Slowed"));
    CheckGranted(Freeze, GAS_DEBUFF_FROZEN, TEXT("Freeze grants GAS.Debuff.Frozen"));
    CheckGranted(Stun, GAS_DEBUFF_STUNNED, TEXT("Stun grants GAS.Debuff.Stunned"));
    CheckGranted(Weaken, GAS_DEBUFF_WEAKENED, TEXT("Weaken grants GAS.Debuff.Weakened"));
    CheckGranted(Terrify, GAS_DEBUFF_TERRIFIED, TEXT("Terrify grants GAS.Debuff.Terrified"));

    TestEqual(TEXT("threshold @0 resistance = 100"), Def::ComputeBuildupThreshold(0.0f), 100.0f);
    TestEqual(TEXT("threshold @full resistance = 102"), Def::ComputeBuildupThreshold(1.0f), 102.0f);
    TestEqual(TEXT("threshold @0.5 resistance = 101"), Def::ComputeBuildupThreshold(0.5f), 101.0f);
    TestEqual(TEXT("threshold clamps resistance > 1 to 102"), Def::ComputeBuildupThreshold(5.0f), 102.0f);
    TestEqual(TEXT("threshold clamps negative resistance to 100"), Def::ComputeBuildupThreshold(-3.0f), 100.0f);

    TestFalse(TEXT("below base threshold does not cross"), Def::BuildupCrossesThreshold(99.0f, 0.0f));
    TestTrue(TEXT("exactly at base threshold crosses"), Def::BuildupCrossesThreshold(100.0f, 0.0f));
    TestTrue(TEXT("above base threshold crosses"), Def::BuildupCrossesThreshold(150.0f, 0.0f));
    TestFalse(TEXT("101 does not cross the 102 threshold at full resistance"), Def::BuildupCrossesThreshold(101.0f, 1.0f));
    TestTrue(TEXT("102 crosses the 102 threshold at full resistance"), Def::BuildupCrossesThreshold(102.0f, 1.0f));

    return true;
}
