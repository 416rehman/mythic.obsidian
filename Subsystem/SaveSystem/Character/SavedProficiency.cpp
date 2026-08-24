#include "SavedProficiency.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyDefinition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "Mythic/Mythic.h"

void FSerializedProficiencyHelper::Serialize(UProficiencyComponent *Component, TArray<FSerializedProficiencyData> &OutData) {
    if (!Component) {
        return;
    }

    UAbilitySystemComponent *ASC = nullptr;
    if (AActor *Owner = Component->GetOwner()) {
        if (IAbilitySystemInterface *ASCInterface = Cast<IAbilitySystemInterface>(Owner)) {
            ASC = ASCInterface->GetAbilitySystemComponent();
        }
    }

    OutData.Empty();

    for (const FProficiency &Prof : Component->Proficiencies) {
        FSerializedProficiencyData Data;

        if (Prof.Definition) {
            Data.ProficiencyAsset = FSoftObjectPath(Prof.Definition);
        }

        if (Prof.ProgressAttribute.IsValid()) {
            if (UStruct *AttrSet = Prof.ProgressAttribute.GetAttributeSetClass()) {
                Data.ProgressAttributeSetClass = AttrSet->GetPathName();
            }
            Data.ProgressAttributeName = Prof.ProgressAttribute.GetName();

            if (ASC) {
                Data.CurrentXP = ASC->GetNumericAttribute(Prof.ProgressAttribute);
            }
        }

        OutData.Add(Data);

        UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Serialize - %s (XP: %.1f)"),
               Prof.Definition ? *Prof.Definition->GetName() : TEXT("NULL"), Data.CurrentXP);
    }
}

void FSerializedProficiencyHelper::Deserialize(UProficiencyComponent *Component, const TArray<FSerializedProficiencyData> &InData) {
    if (!Component) {
        return;
    }

    // The authored defaults survive the load: a save knows the XP of the tracks it recorded, never the roster.
    // Replacing the roster with the save's list made every track vanish for a fresh character (empty stub save)
    // and would silently drop any proficiency added to the game after a save was written.
    TArray<FProficiency> Authored = MoveTemp(Component->Proficiencies);
    Component->Proficiencies.Empty();

    for (const FSerializedProficiencyData &Data : InData) {
        FProficiency Prof;

        Prof.Definition = Cast<UProficiencyDefinition>(Data.ProficiencyAsset.TryLoad());
        if (!Prof.Definition) {
            UE_LOG(MythSaveLoad, Error, TEXT("SavedProficiency::Deserialize - Failed to load definition: %s"),
                   *Data.ProficiencyAsset.ToString());
            continue;
        }

        if (!Data.ProgressAttributeSetClass.IsEmpty() && !Data.ProgressAttributeName.IsEmpty()) {
            UClass *SetClass = LoadClass<UAttributeSet>(nullptr, *Data.ProgressAttributeSetClass);
            if (SetClass) {
                FProperty *Prop = SetClass->FindPropertyByName(FName(*Data.ProgressAttributeName));
                if (Prop) {
                    Prof.ProgressAttribute = FGameplayAttribute(Prop);
                }
                else {
                    UE_LOG(MythSaveLoad, Error, TEXT("SavedProficiency::Deserialize - Property not found: %s in %s"),
                           *Data.ProgressAttributeName, *Data.ProgressAttributeSetClass);
                }
            }
            else {
                UE_LOG(MythSaveLoad, Error, TEXT("SavedProficiency::Deserialize - Class not found: %s"),
                       *Data.ProgressAttributeSetClass);
            }
        }

        Prof.SavedXP = Data.CurrentXP;

        Component->Proficiencies.Add(Prof);

        UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %s (XP: %.1f)"),
               *Prof.Definition->GetName(), Prof.SavedXP);
    }

    for (FProficiency &Default : Authored) {
        const bool bRestored = Default.Definition && Component->Proficiencies.ContainsByPredicate(
            [&Default](const FProficiency &P) { return P.Definition == Default.Definition; });
        if (Default.Definition && !bRestored) {
            Component->Proficiencies.Add(MoveTemp(Default));
        }
    }

    UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %d proficiencies"), Component->Proficiencies.Num());
}
