#include "CharacterData.h"
#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"

bool FSerializedCharacterData::Serialize(AActor *SourceActor, FSerializedCharacterData &OutData) {
    if (!SourceActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Serialize: Invalid SourceActor"));
        return false;
    }

    // Character Name
    if (APlayerState *PS = Cast<APlayerState>(SourceActor)) {
        OutData.CharacterName = PS->GetPlayerName();
    }

    // Proficiencies (save first - they track claimed levels)
    if (UProficiencyComponent *ProfComp = SourceActor->FindComponentByClass<UProficiencyComponent>()) {
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found ProficiencyComponent, serializing %d proficiencies..."),
               ProfComp->Proficiencies.Num());
        FSerializedProficiencyHelper::Serialize(ProfComp, OutData.Proficiencies);
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d proficiencies"), OutData.Proficiencies.Num());
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: No ProficiencyComponent found!"));
    }

    // Inventory - use IInventoryProviderInterface
    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(SourceActor)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found %d inventory components"), InventoryComponents.Num());

        for (UMythicInventoryComponent *InventoryComp : InventoryComponents) {
            if (InventoryComp) {
                FSerializedInventoryData InvData;
                FSerializedInventoryData::Serialize(InventoryComp, InvData);
                OutData.Inventories.Add(InvData);
                UE_LOG(MythSaveLoad, Log, TEXT("  - Serialized inventory '%s' with %d slots"), *InventoryComp->GetName(), InvData.Slots.Num());
            }
        }
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: SourceActor does not implement IInventoryProviderInterface!"));
    }

    // Note: Attributes are NOT saved - they are derived from proficiencies + items on load

    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized character '%s'"), *OutData.CharacterName);
    return true;
}

bool FSerializedCharacterData::Deserialize(AActor *TargetActor, const FSerializedCharacterData &InData) {
    if (!TargetActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Deserialize: Invalid TargetActor"));
        return false;
    }

    // Character Name
    if (APlayerState *PS = Cast<APlayerState>(TargetActor)) {
        PS->SetPlayerName(InData.CharacterName);
    }

    // Proficiencies (load first - they will reapply rewards in BeginPlay)
    if (UProficiencyComponent *ProfComp = TargetActor->FindComponentByClass<UProficiencyComponent>()) {
        FSerializedProficiencyHelper::Deserialize(ProfComp, InData.Proficiencies);
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d proficiencies"), ProfComp->Proficiencies.Num());
    }

    // Inventory (items will apply their stats when equipped)
    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(TargetActor)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        for (int32 i = 0; i < InventoryComponents.Num() && i < InData.Inventories.Num(); ++i) {
            if (InventoryComponents[i]) {
                FSerializedInventoryData::Deserialize(InventoryComponents[i], InData.Inventories[i]);
            }
        }
    }

    // Note: Attributes are derived - proficiencies reapply rewards, items apply stats when equipped

    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Applied character '%s'"), *InData.CharacterName);
    return true;
}
