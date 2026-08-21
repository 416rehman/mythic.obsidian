#include "MythicSaveGame.h"
#include "Mythic/Mythic.h"

void UMythicSaveGame::FixupData() {
    if (CharacterData.DataVersion < static_cast<int32>(CurrentCharacterSaveVersion)) {
        UE_LOG(Myth, Log, TEXT("Migrating Save Data from Version %d to %d"), CharacterData.DataVersion, (int32)CurrentCharacterSaveVersion);


        CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    }
}
