#include "MythicWeaponActionContract.h"
#include "AttackFragment.h"
#include "WeaponBlockFragment.h"
#include "WeaponSecondaryAttackFragment.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Input/MythicTags_Input.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/InventoryProviderInterface.h"
namespace MythicWeaponAction {
bool ValidateDefinition(const UItemDefinition* Definition,FText* Error) {
    auto Fail=[Error](const TCHAR* Why){if(Error)*Error=FText::FromString(Why);return false;};
    if (!Definition) return Fail(TEXT("Missing item definition"));
    int32 Primary=0, Secondary=0;
    for (const auto& Fragment:Definition->Fragments) {
        if (const auto* Attack=Cast<UAttackFragment>(Fragment)) {
            (Attack->IsSecondaryAttack()?Secondary:Primary)++;
            if (const auto* Heavy=Cast<UWeaponSecondaryAttackFragment>(Attack); Heavy && !Heavy->HasValidHeavyTuning())
                return Fail(TEXT("Invalid immutable heavy attack tuning"));
            if (Attack->InputTag!=Attack->GetCanonicalInputTag()) return Fail(TEXT("Attack role has a noncanonical input tag"));
        } else if (const auto* Block=Cast<UWeaponBlockFragment>(Fragment)) {
            ++Secondary;
            if (Block->InputTag!=INPUT_ACTION_ATTACK_ALT || !Block->HasUsableEffect()) return Fail(TEXT("Invalid held defense contract"));
        }
    }
    const bool bWeapon=Definition->ItemType.MatchesTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON);
    if (Primary>1 || Secondary>1 || (Secondary && !bWeapon) || (bWeapon && Primary!=1))
        return Fail(TEXT("A weapon needs exactly one primary and at most one heavy OR held defense; non-weapons cannot grant a secondary"));
    return true;
}
bool MatchesDefinition(const UItemFragment* Fragment) {
    const auto* Item=Fragment?Fragment->GetOwningItemInstance():nullptr;
    const auto* Definition=IsValid(Item)?Item->GetItemDefinition():nullptr;
    if (!IsValid(Fragment) || !Item || !Item->ContainsFragment(Fragment) || !ValidateDefinition(Definition)) return false;
    const UItemFragment* Template=nullptr; int32 Count=0;
    for (const auto& Entry:Definition->Fragments) if (Entry && Entry->GetClass()==Fragment->GetClass()) {Template=Entry; ++Count;}
    if (Count!=1) return false;
    int32 LiveCount=0;
    for (const auto& Entry:Item->GetFragmentsView()) if (Entry && Entry->GetClass()==Fragment->GetClass()) ++LiveCount;
    if (LiveCount!=1) return false;
    // Exact class is deliberate: first-derived-match would let one source borrow another action's immutable config.
    if (const auto* Attack=Cast<UAttackFragment>(Fragment)) {
        const auto* Authored=Cast<UAttackFragment>(Template);
        if (const auto* Heavy=Cast<UWeaponSecondaryAttackFragment>(Attack)) {
            const auto* AuthoredHeavy=Cast<UWeaponSecondaryAttackFragment>(Template);
            if (!AuthoredHeavy || !Heavy->HasValidHeavyTuning()
                || Heavy->DamageMultiplier!=AuthoredHeavy->DamageMultiplier
                || Heavy->AttackRateMultiplier!=AuthoredHeavy->AttackRateMultiplier) return false;
        }
        return Authored && Attack->IsSecondaryAttack()==Authored->IsSecondaryAttack()
            && Attack->AttackConfig.TriggerAbility==Authored->AttackConfig.TriggerAbility
            && Attack->AttackConfig.AttackMontage==Authored->AttackConfig.AttackMontage;
    }
    if (const auto* Block=Cast<UWeaponBlockFragment>(Fragment)) {
        const auto* Authored=Cast<UWeaponBlockFragment>(Template);
        return Authored && Block->GuardEffect==Authored->GuardEffect && Block->HasUsableEffect();
    }
    return false;
}
bool IsEquippedSource(const UItemFragment* Fragment,const FGameplayAbilityActorInfo* Info) {
    if (!Info || !MatchesDefinition(Fragment)) return false;
    const auto* Item=Fragment->GetOwningItemInstance(); auto* Inventory=Item->GetInventoryComponent();
    if (!Inventory) return false;
    const auto& Slots=Inventory->GetAllSlots(); const int32 Slot=Item->GetSlot();
    if (!Slots.IsValidIndex(Slot) || !Slots[Slot].AppliesEquipmentEffects() || Slots[Slot].SlottedItemInstance.Get()!=Item) return false;
    if (const auto* Durability=Item->GetFragment<UDurabilityFragment>(); Durability && Durability->IsBroken()) return false;
    AActor* Candidates[]={Info->OwnerActor.Get(),Info->AvatarActor.Get(),Info->PlayerController.Get()};
    for (AActor* Candidate:Candidates) {
        const auto* Provider=Cast<IInventoryProviderInterface>(Candidate);
        if (!Provider) if (const auto* Pawn=Cast<APawn>(Candidate)) Provider=Cast<IInventoryProviderInterface>(Pawn->GetController());
        if (!Provider) if (const auto* State=Cast<APlayerState>(Candidate)) Provider=Cast<IInventoryProviderInterface>(State->GetPlayerController());
        if (Provider && Provider->GetAllInventoryComponents().Contains(Inventory)) return true;
    }
    return false;
}
}
