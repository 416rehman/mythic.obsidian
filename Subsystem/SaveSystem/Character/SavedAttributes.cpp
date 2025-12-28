#include "SavedAttributes.h"
#include "AbilitySystemComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Mythic/Itemization/Inventory/Fragments/ItemFragment.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"

void FSerializedAttributesData::Serialize(UAbilitySystemComponent *ASC, FSerializedAttributesData &OutData) {
    if (!ASC) {
        return;
    }

    UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(ASC);
    if (!MythicASC) {
        return;
    }

    OutData.Attributes.Empty();
    OutData.GrantedAbilities.Empty();

    // Attributes
    const TArray<UAttributeSet *> &SpawnedAttributes = MythicASC->GetSpawnedAttributes();
    for (UAttributeSet *Set : SpawnedAttributes) {
        if (!Set) {
            continue;
        }
        for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It) {
            if (FStructProperty *StructProp = CastField<FStructProperty>(*It)) {
                if (StructProp->Struct == FGameplayAttributeData::StaticStruct()) {
                    FGameplayAttribute Attr(*It);
                    if (Attr.IsValid()) {
                        float Val = MythicASC->GetNumericAttributeBase(Attr);
                        OutData.Attributes.Add(It->GetName(), Val);
                    }
                }
            }
        }
    }

    // Abilities
    for (const FGameplayAbilitySpec &Spec : MythicASC->GetActivatableAbilities()) {
        if (Spec.Ability) {
            // Skip abilities granted by items - they will be re-granted when items are activated on load
            if (Spec.SourceObject.IsValid()) {
                if (Spec.SourceObject->IsA<UItemFragment>() || Spec.SourceObject->IsA<UMythicItemInstance>()) {
                    continue;
                }
            }
            OutData.GrantedAbilities.Add(FSoftClassPath(Spec.Ability->GetClass()));
        }
    }
}

void FSerializedAttributesData::Deserialize(UAbilitySystemComponent *ASC, const FSerializedAttributesData &InData) {
    if (!ASC) {
        return;
    }

    UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(ASC);
    if (!MythicASC) {
        return;
    }

    // Attributes
    const TArray<UAttributeSet *> &SpawnedAttributes = MythicASC->GetSpawnedAttributes();
    for (const auto &Pair : InData.Attributes) {
        const FString &AttrName = Pair.Key;
        float Val = Pair.Value;

        for (UAttributeSet *Set : SpawnedAttributes) {
            if (!Set) {
                continue;
            }
            FProperty *Prop = Set->GetClass()->FindPropertyByName(FName(*AttrName));
            if (FStructProperty *StructProp = CastField<FStructProperty>(Prop)) {
                if (StructProp->Struct == FGameplayAttributeData::StaticStruct()) {
                    FGameplayAttribute Attr(Prop);
                    if (Attr.IsValid()) {
                        MythicASC->SetNumericAttributeBase(Attr, Val);
                        break;
                    }
                }
            }
        }
    }

    // Abilities
    for (const FSoftClassPath &AbilityPath : InData.GrantedAbilities) {
        if (UClass *AbilityClass = AbilityPath.TryLoadClass<UGameplayAbility>()) {
            FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, MythicASC->GetOwner());
            MythicASC->GiveAbility(Spec);
        }
    }
}
