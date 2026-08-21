
#include "Misc/AutomationTest.h"

#include "GameplayEffectTypes.h"
#include "GameplayTagsManager.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHitTagTest,
    "Mythic.Combat.HitTags",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHitTagTest::RunTest(const FString &Parameters) {
    // An unregistered tag still passes IsValid() in the editor, so ask the manager.
    TestTrue(TEXT("the critical hit tag is registered"),
             UGameplayTagsManager::Get().RequestGameplayTag(GAS_HIT_CRITICAL.GetTag().GetTagName(), false).IsValid());

    {
        FMythicGameplayEffectContext *Ctx = new FMythicGameplayEffectContext();
        Ctx->SetCriticalHit(true);
        const FGameplayEffectContextHandle Handle(Ctx);

        FGameplayTagContainer Tags;
        UMythicAttributeSet_Life::AppendHitTags(Handle, Tags);
        TestTrue(TEXT("a critical hit carries the crit tag"), Tags.HasTag(GAS_HIT_CRITICAL));
    }

    {
        FMythicGameplayEffectContext *Ctx = new FMythicGameplayEffectContext();
        Ctx->SetCriticalHit(false);
        const FGameplayEffectContextHandle Handle(Ctx);

        FGameplayTagContainer Tags;
        UMythicAttributeSet_Life::AppendHitTags(Handle, Tags);
        TestFalse(TEXT("an ordinary hit does not"), Tags.HasTag(GAS_HIT_CRITICAL));
    }

    {
        // A plain GAS context is not a Mythic one; reading it must not crash or invent a tag.
        const FGameplayEffectContextHandle Plain(new FGameplayEffectContext());
        FGameplayTagContainer Tags;
        UMythicAttributeSet_Life::AppendHitTags(Plain, Tags);
        TestEqual(TEXT("a non-Mythic context adds nothing"), Tags.Num(), 0);

        FGameplayTagContainer Empty;
        UMythicAttributeSet_Life::AppendHitTags(FGameplayEffectContextHandle(), Empty);
        TestEqual(TEXT("an empty context adds nothing"), Empty.Num(), 0);
    }

    return true;
}
