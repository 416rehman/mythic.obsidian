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

    // Get ASC to read current XP values
    UAbilitySystemComponent *ASC = nullptr;
    if (AActor *Owner = Component->GetOwner()) {
        if (IAbilitySystemInterface *ASCInterface = Cast<IAbilitySystemInterface>(Owner)) {
            ASC = ASCInterface->GetAbilitySystemComponent();
        }
    }

    OutData.Empty();

    for (const FProficiency &Prof : Component->Proficiencies) {
        FSerializedProficiencyData Data;

        // Save the proficiency definition asset reference
        if (Prof.Definition) {
            Data.ProficiencyAsset = FSoftObjectPath(Prof.Definition);
        }

        // Save progress attribute identity as strings
        if (Prof.ProgressAttribute.IsValid()) {
            if (UStruct *AttrSet = Prof.ProgressAttribute.GetAttributeSetClass()) {
                Data.ProgressAttributeSetClass = AttrSet->GetPathName();
            }
            Data.ProgressAttributeName = Prof.ProgressAttribute.GetName();

            // Get current XP from ASC
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

    Component->Proficiencies.Empty();

    for (const FSerializedProficiencyData &Data : InData) {
        FProficiency Prof;

        // Load the proficiency definition
        Prof.Definition = Cast<UProficiencyDefinition>(Data.ProficiencyAsset.TryLoad());
        if (!Prof.Definition) {
            UE_LOG(MythSaveLoad, Error, TEXT("SavedProficiency::Deserialize - Failed to load definition: %s"),
                   *Data.ProficiencyAsset.ToString());
            continue;
        }

        // Reconstruct the progress attribute from strings
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

        // Store XP to apply in BeginPlay (ASC not ready yet)
        // ClaimedLevels will be derived from XP in BeginPlay
        Prof.SavedXP = Data.CurrentXP;

        Component->Proficiencies.Add(Prof);

        UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %s (XP: %.1f)"),
               *Prof.Definition->GetName(), Prof.SavedXP);
    }

    UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %d proficiencies"), Component->Proficiencies.Num());
}
