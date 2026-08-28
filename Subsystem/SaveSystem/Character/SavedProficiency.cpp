#include "SavedProficiency.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyDefinition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic/Mythic.h"

bool FSerializedProficiencyHelper::Serialize(
    UProficiencyComponent *Component,
    TArray<FSerializedProficiencyData> &OutData) {
    if (!Component) {
        return false;
    }

    UAbilitySystemComponent *ASC = nullptr;
    if (AActor *Owner = Component->GetOwner()) {
        if (IAbilitySystemInterface *ASCInterface = Cast<IAbilitySystemInterface>(Owner)) {
            ASC = ASCInterface->GetAbilitySystemComponent();
        }
    }

    if (!ASC) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SavedProficiency::Serialize - authoritative ASC is unavailable."));
        return false;
    }

    TArray<FSerializedProficiencyData> StagedData;
    TSet<const UProficiencyDefinition *> SeenDefinitions;

    for (const FProficiency &Prof : Component->Proficiencies) {
        const FGameplayAttribute ProgressAttribute = Prof.GetProgressAttribute();
        if (!Prof.Definition || SeenDefinitions.Contains(Prof.Definition)
            || !ProgressAttribute.IsValid()
            || !ASC->HasAttributeSetForAttribute(ProgressAttribute)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("SavedProficiency::Serialize - rejected track without a typed, installed Progress Stat: %s"),
                   *GetNameSafe(Prof.Definition));
            return false;
        }
        SeenDefinitions.Add(Prof.Definition);

        FSerializedProficiencyData Data;
        Data.ProficiencyDefinition = Prof.Definition;
        Data.CurrentXP = ASC->GetNumericAttributeBase(ProgressAttribute);
        if (!FMath::IsFinite(Data.CurrentXP) || Data.CurrentXP < 0.0f) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("SavedProficiency::Serialize - rejected non-finite or negative XP for %s."),
                   *GetNameSafe(Prof.Definition));
            return false;
        }

        StagedData.Add(Data);

        UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Serialize - %s (XP: %.1f)"),
               Prof.Definition ? *Prof.Definition->GetName() : TEXT("NULL"), Data.CurrentXP);
    }
    OutData = MoveTemp(StagedData);
    return true;
}

bool FSerializedProficiencyHelper::Deserialize(
    UProficiencyComponent *Component,
    const TArray<FSerializedProficiencyData> &InData) {
    if (!Component) {
        return false;
    }

    TMap<FSoftObjectPath, int32> RosterIndexByDefinition;
    for (int32 Index = 0; Index < Component->Proficiencies.Num(); ++Index) {
        const FProficiency &Proficiency = Component->Proficiencies[Index];
        if (!Proficiency.Definition) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("SavedProficiency::Deserialize - authored roster contains a null definition."));
            return false;
        }
        const FSoftObjectPath DefinitionPath(Proficiency.Definition);
        if (!DefinitionPath.IsValid() || RosterIndexByDefinition.Contains(DefinitionPath)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("SavedProficiency::Deserialize - authored roster contains a duplicate definition: %s."),
                   *DefinitionPath.ToString());
            return false;
        }
        RosterIndexByDefinition.Add(DefinitionPath, Index);
    }

    TMap<FSoftObjectPath, float> SavedXpByDefinition;
    for (const FSerializedProficiencyData &Data : InData) {
        const FSoftObjectPath DefinitionPath = Data.ProficiencyDefinition.ToSoftObjectPath();
        if (!DefinitionPath.IsValid() || !FMath::IsFinite(Data.CurrentXP)
            || Data.CurrentXP < 0.0f || SavedXpByDefinition.Contains(DefinitionPath)
            || !RosterIndexByDefinition.Contains(DefinitionPath)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("SavedProficiency::Deserialize - rejected invalid or duplicate typed entry: %s"),
                   *DefinitionPath.ToString());
            return false;
        }
        SavedXpByDefinition.Add(DefinitionPath, Data.CurrentXP);
    }

    // The authored component owns the roster. Save data can restore XP for an existing typed definition, but it
    // cannot inject arbitrary tracks or duplicate the same track under a reconstructed attribute identity.
    TArray<float> StagedXp;
    StagedXp.Init(0.0f, Component->Proficiencies.Num());
    for (const TPair<FSoftObjectPath, float> &Pair : SavedXpByDefinition) {
        StagedXp[RosterIndexByDefinition.FindChecked(Pair.Key)] = Pair.Value;
    }
    for (int32 Index = 0; Index < Component->Proficiencies.Num(); ++Index) {
        FProficiency &Proficiency = Component->Proficiencies[Index];
        Proficiency.SavedXP = StagedXp[Index];
        if (SavedXpByDefinition.Contains(FSoftObjectPath(Proficiency.Definition))) {
            UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %s (XP: %.1f)"),
                   *Proficiency.Definition->GetName(), Proficiency.SavedXP);
        }
    }

    UE_LOG(MythSaveLoad, Log, TEXT("SavedProficiency::Deserialize - Restored %d proficiencies"), Component->Proficiencies.Num());
    return true;
}
