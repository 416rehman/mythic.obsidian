// 


#include "AttackFragment.h"

#include "GameModes/GameState/MythicGameState.h"
#include "GameFramework/Pawn.h"                        // resolve the equipping player's controller
#include "Player/MythicPlayerController.h"             // NotifyItemEquipped -> "equip N <type>" objectives
#include "Itemization/Inventory/MythicItemInstance.h"  // GetItemDefinition for the equip event payload

class AMythicGameState;

#if WITH_EDITOR
bool UAttackFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!this->AttackConfig.TriggerAbility) {
        OutErrorMessage = FText::FromString("AttackFragment: TriggerAbility is not set");
        return false;
    }

    if (!this->AttackConfig.AttackMontage) {
        OutErrorMessage = FText::FromString("AttackFragment: AttackMontage is not set");
        return false;
    }

    if (!this->AttackBuildData.DamageRollDefinition.IsValid(OutErrorMessage)) {
        return false;
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif

void UAttackFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnInstanced: Fragment=%s, Item=%s, InputTag=%s, TriggerAbility=%s"),
           *GetName(),
           *GetNameSafe(Instance),
           *InputTag.ToString(),
           *GetNameSafe(AttackConfig.TriggerAbility));

    // Roll the min/max damage values, make sure min is less than max
    this->AttackRuntimeReplicatedData.RolledDamageSpec = FRolledAttributeSpec(AttackBuildData.DamageAttribute, Instance->GetItemLevel(),
                                                                              AttackBuildData.DamageRollDefinition);
}

void UAttackFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemActivated: Fragment=%s, Item=%s, InputTag=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance),
           *InputTag.ToString());

    this->AttackRuntimeReplicatedData.ASC = this->GetOwningAbilitySystemComponent();
    if (!this->AttackRuntimeReplicatedData.ASC) {
        UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: ASC is null! Cannot grant ability."));
        return;
    }

    UE_LOG(Myth, Log, TEXT("  -> Got ASC: %s (Owner: %s)"),
           *GetNameSafe(AttackRuntimeReplicatedData.ASC),
           *GetNameSafe(AttackRuntimeReplicatedData.ASC->GetOwnerActor()));

    const auto ASC = this->AttackRuntimeReplicatedData.ASC;
    auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;

    // Decide each action independently. The old single early-return on bIsApplied also skipped the ability grant —
    // so a re-activation whose first grant had failed (damage applied, no live ability) would never get an attack
    // ability. Damage and ability are now each idempotent on their own state.
    const FAttackActivationPlan Plan = PlanAttackActivation(Roll->bIsApplied, this->AttackRuntimeReplicatedData.AbilityHandle.IsValid());

    if (Plan.bApplyDamage) {
        ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, Roll->Value);
        Roll->bIsApplied = true;
        UE_LOG(Myth, Log, TEXT("  -> Applied damage attribute: %s = %f"), *Roll->Attribute.GetName(), Roll->Value);
    }
    else {
        UE_LOG(Myth, Log, TEXT("  -> Damage already applied, skipping attribute modification"));
    }

    if (Plan.bGrantAbility) {
        // Give the player the ability to attack
        UE_LOG(Myth, Log, TEXT("  -> Granting ability %s with InputTag %s"), *GetNameSafe(AttackConfig.TriggerAbility), *InputTag.ToString());
        this->AttackRuntimeReplicatedData.AbilityHandle = GrantItemAbility(ASC, ItemInstance, AttackConfig.TriggerAbility);

        if (!this->AttackRuntimeReplicatedData.AbilityHandle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UAttackFragment::OnItemActivated: Failed to grant attack ability %s"), *GetNameSafe(AttackConfig.TriggerAbility));
        }
        else {
            UE_LOG(Myth, Log, TEXT("  -> SUCCESS! Granted ability"));

            // Log the granted ability's dynamic tags to verify the InputTag was applied
            if (FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(this->AttackRuntimeReplicatedData.AbilityHandle)) {
                FGameplayTagContainer DynamicTags = Spec->GetDynamicSpecSourceTags();
                UE_LOG(Myth, Log, TEXT("  -> Granted ability DynamicSpecSourceTags: %s"), *DynamicTags.ToString());
            }
        }
    }
    else {
        UE_LOG(Myth, Log, TEXT("  -> Attack ability already live, skipping grant"));
    }

    // Weapon EQUIPPED → drive "equip N <type>" objectives, ONCE per genuine equip. Gated on the per-item SaveGame marker
    // (NOT bGrantAbility): idempotent across re-activation AND save-restore (a restored equipped weapon has marker==true →
    // no re-emit, no over-count). Reset on unequip so a real re-equip emits again. Equipping never destroys the instance,
    // so reading its definition here is safe. Resolve the owning player via the avatar's controller (AI casts to null).
    // The marker is per-FRAGMENT but OnActiveItem runs OnItemActivated on EVERY fragment, so a (malformed) weapon with two
    // UAttackFragments would emit twice for one equip. Guard on identity: only the CANONICAL first attack fragment (the one
    // GetFragment<T> returns — what every other consumer reads) emits, so the equip counts once regardless of fragment count.
    const bool bIsCanonicalAttackFragment = ItemInstance && ItemInstance->GetFragment<UAttackFragment>() == this;
    if (bIsCanonicalAttackFragment && !this->AttackRuntimeReplicatedData.bEquipEventEmitted) {
        this->AttackRuntimeReplicatedData.bEquipEventEmitted = true;
        if (AActor *Avatar = ASC->GetAvatarActor()) {
            if (const APawn *Pawn = Cast<APawn>(Avatar)) {
                if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Pawn->GetController())) {
                    PC->NotifyItemEquipped(ItemInstance->GetItemDefinition());
                }
            }
        }
    }
}

void UAttackFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    UE_LOG(Myth, Log, TEXT("UAttackFragment::OnItemDeactivated: Fragment=%s, Item=%s"),
           *GetName(),
           *GetNameSafe(ItemInstance));

    // Unequipped → re-arm the equip event so the NEXT genuine equip emits again. MUST be before the !bIsApplied early-return
    // below, so the marker always resets on unequip regardless of the damage-state branch.
    this->AttackRuntimeReplicatedData.bEquipEventEmitted = false;

    auto ASC = this->AttackRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Log, TEXT("  -> No ASC cached, nothing to clean up"));
        return;
    }

    const auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    if (!Roll->bIsApplied) {
        UE_LOG(Myth, Log, TEXT("  -> Damage not applied, nothing to reverse"));
        return;
    }

    // Reverse the damage attribute
    ASC->ApplyModToAttribute(Roll->Attribute, EGameplayModOp::AddBase, -Roll->Value);
    Roll->bIsApplied = false;
    UE_LOG(Myth, Log, TEXT("  -> Reversed damage attribute"));

    // Remove the ability
    auto AbilityHandle = this->AttackRuntimeReplicatedData.AbilityHandle;
    if (AttackConfig.TriggerAbility && AbilityHandle.IsValid()) {
        ASC->ClearAbility(AbilityHandle);
        UE_LOG(Myth, Log, TEXT("  -> Cleared ability"));
    }

    // Invalidate the MEMBER handle so a later re-equip is recognized as a fresh grant. ClearAbility removes the spec from
    // the ASC but does NOT invalidate the caller's handle value — without this reset, the stale member handle still reads
    // IsValid()==true, so PlanAttackActivation (which gates the re-grant on AbilityHandle.IsValid()) returns bGrantAbility
    // = false on re-equip, and the re-equipped weapon never gets its attack ability back (unarmed after unequip/re-equip).
    this->AttackRuntimeReplicatedData.AbilityHandle = FGameplayAbilitySpecHandle();

    // Remove the ability component
    this->AttackRuntimeReplicatedData.ASC = nullptr;
}


bool UAttackFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherFragment = Cast<UAttackFragment>(Other);
    if (!OtherFragment) {
        return false;
    }

    auto Roll = &this->AttackRuntimeReplicatedData.RolledDamageSpec;
    auto OtherRoll = &OtherFragment->AttackRuntimeReplicatedData.RolledDamageSpec;

    auto OtherConfig = OtherFragment->AttackConfig;

    return Super::CanBeStackedWith(Other) &&
        Roll->Attribute == OtherRoll->Attribute &&
        Roll->Value == OtherRoll->Value &&
        AttackConfig.TriggerAbility == OtherConfig.TriggerAbility &&
        AttackConfig.AttackMontage == OtherConfig.AttackMontage;
}
