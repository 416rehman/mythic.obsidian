#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestProbe.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"

inline UMythicSkillDefinition* MakeAbilityWindowTestDefinition(UObject* Outer,FGameplayTag CooldownTag) {
    auto* Definition=NewObject<UMythicSkillDefinition>(Outer);
    Definition->Name=FText::FromString(TEXT("Automation action probe"));
    Definition->Description=FText::FromString(TEXT("Test fixture for production action admission."));
    Definition->Hint=FText::FromString(TEXT("Automation only"));
    Definition->Icon=FSoftObjectPath(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
    Definition->RuntimeAbility=UMythicAbilityWindowTestProbe::StaticClass();
    Definition->CooldownEffect=UMythicAbilityTestCooldown::StaticClass();
    Definition->CooldownTag=CooldownTag; Definition->CooldownSeconds=10;
    Definition->bHasMasteryTrack=true; Definition->MasteryUseThresholds={10,25};
    FMythicAbilityActionProfile& Action=Definition->Actions.AddDefaulted_GetRef();
    Action.ActionId=TEXT("Probe"); Action.MinimumDamage=Action.MaximumDamage=10; Action.bScaleWithPrimaryStats=false;
    for (const auto* Tag:{TEXT("Ability.Action.Reach.Ranged"),TEXT("Ability.Action.Element.Physical"),TEXT("Ability.Action.Effect.Damage")})
        Action.Facets.AddTag(FGameplayTag::RequestGameplayTag(Tag));
    Action.Damage.DamageCalculationEffect=LoadClass<UGameplayEffect>(nullptr,TEXT("/Game/Mythic/GAS/Damage/GE_CalculateDamage.GE_CalculateDamage_C"));
    Action.Damage.DamageApplicationEffect=LoadClass<UGameplayEffect>(nullptr,TEXT("/Game/Mythic/GAS/Damage/GE_ApplyDamage.GE_ApplyDamage_C"));
    return Definition;
}

inline FGameplayAbilitySpec* FindAbilityTestSpec(UMythicAbilitySystemComponent* ASC,const UMythicSkillDefinition* Definition) {
    for (auto& Spec:ASC->GetActivatableAbilities()) if (Spec.SourceObject.Get()==Definition) return &Spec;
    return nullptr;
}
#endif
