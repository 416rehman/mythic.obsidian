#include "AbilityReward.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicSkillComponent.h"

bool UAbilityReward::Give(FRewardContext& Context) const {
    APlayerController* PC = Context.PlayerController;
    if (!IsValid(PC) || !PC->HasAuthority() || (SkillDefinition != nullptr) == (Ability != nullptr)) return false;
    AMythicPlayerState* PS = PC->GetPlayerState<AMythicPlayerState>();
    if (!PS) return false;
    if (SkillDefinition) return PS->GetSkillComponent() && PS->GetSkillComponent()->AcceptRewardSkill(SkillDefinition);
    UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
    if (!ASC) return false;
    if (ASC->FindAbilitySpecFromClass(Ability)) return true;
    const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(Ability,1,INDEX_NONE,const_cast<UAbilityReward*>(this)));
    if (!Handle.IsValid()) return false;
    if (Activate) ASC->TryActivateAbility(Handle);
    return true;
}
FText UAbilityReward::GetPreviewText() const {
    if (SkillDefinition) return FText::Format(NSLOCTEXT("Mythic","LearnAbilityReward","Learn {0}"),SkillDefinition->Name);
    return Ability ? FText::FromString(Ability->GetName()) : FText::GetEmpty();
}
