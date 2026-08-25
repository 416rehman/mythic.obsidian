#include "MythicSaveGame.h"
#include "Mythic/Mythic.h"

void UMythicSaveGame::FixupData() {
    if (CharacterData.DataVersion < static_cast<int32>(CurrentCharacterSaveVersion)) {
        UE_LOG(Myth, Log, TEXT("Migrating Save Data from Version %d to %d"), CharacterData.DataVersion, (int32)CurrentCharacterSaveVersion);

        if (CharacterData.DataVersion <= static_cast<int32>(EMythicCharacterSaveVersion::PreRunes)) {
            CharacterData.UnlockedRuneSlots = FMythicCharacterSaveMigration::RuneSlotsFromAppliedRules(
                CharacterData.AppliedUnlockRules);
        }

        if (CharacterData.DataVersion <= static_cast<int32>(EMythicCharacterSaveVersion::PreSkills)) {
            CharacterData.UnlockedSkillSlots = FMythicCharacterSaveMigration::SkillSlotsFromAppliedRules(
                CharacterData.AppliedUnlockRules);
        }

        CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    }
}
