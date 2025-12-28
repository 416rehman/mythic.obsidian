#include "MythicSaveGame.h"
#include "Mythic/Mythic.h"

void UMythicSaveGame::FixupData() {
    // Implementation for migration logic
    // e.g. if (CharacterData.DataVersion < CurrentVersion) { ... }

    if (CharacterData.DataVersion < static_cast<int32>(CurrentCharacterSaveVersion)) {
        UE_LOG(Myth, Log, TEXT("Migrating Save Data from Version %d to %d"), CharacterData.DataVersion, (int32)CurrentCharacterSaveVersion);

        // Migration logic goes here

        CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    }
}
