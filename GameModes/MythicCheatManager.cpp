#include "MythicCheatManager.h"
#include "UObject/UObjectIterator.h"
#include "UI/MythicHUDLayout.h"
#include "CommonUIExtensions.h"
#include "UI/Menu/MythicEscapeMenuWidget.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "Mythic/World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Mythic/World/EnvironmentController/MythicEnvironmentController.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Itemization/Inventory/ItemDefinition.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/System/MythicAssetManager.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Player/MythicPlayerController.h"
#include "Mythic/GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Mythic/GameModes/GameState/MythicGameState.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "Mythic/Progression/Skills/MythicSkillComponent.h"
#include "Mythic/Progression/Skills/MythicSkillDefinition.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyDefinition.h"
#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#include "Mythic/GAS/MythicTags_GAS.h"
#include "Mythic/GAS/Effects/MythicStatusRegistry.h"
#include "Mythic/GAS/Effects/MythicStatusEffectDefinition.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicAchievementDefinition.h"
#include "Mythic/Progression/MythicAchievementSet.h"
#include "Mythic/Progression/MythicStatLedgerComponent.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Mythic/Progression/Runes/MythicRuneComponent.h"
#include "Mythic/Progression/Runes/MythicRuneDefinition.h"
#include "Mythic/Itemization/Inventory/Fragments/FragmentTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Mythic/GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "Mythic/GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"
#include "Mythic/Mythic.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/UIActionBinding.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "EnhancedInputSubsystems.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "Mythic/World/LivingWorld/LivingWorldSubsystem.h"
#include "Mythic/World/LivingWorld/Factions/FactionDatabase.h"
#include "Mythic/World/LivingWorld/Territory/TerritoryGrid.h"
#include "Mythic/World/LivingWorld/Settlements/SettlementRegistry.h"
#include "Mythic/World/LivingWorld/CausalFabric/CausalFabric.h"
#include "MassEntitySubsystem.h"
#include "MassEntityQuery.h"
#include "Mythic/Mass/Tags/MythicMassTags.h"
#include "Mythic/Mass/Fragments/MythicMassFragments.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "World/LivingWorld/Debugging/MythicLivingWorldDebugActor.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "AI/Party/PartySubsystem.h"
#include "AI/Party/MythicPartyTypes.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "GameFramework/Pawn.h"


void UMythicCheatManager::MythHelp() {
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== MYTHIC CHEAT COMMANDS ==="));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- SAVE SYSTEM ---"));
    UE_LOG(Myth, Warning, TEXT("  MythSaveCharacter <SlotName>       - Save character to slot"));
    UE_LOG(Myth, Warning, TEXT("  MythLoadCharacter <SlotName>       - Load character from slot"));
    UE_LOG(Myth, Warning, TEXT("  MythSaveWorld <SlotName>           - Save world state"));
    UE_LOG(Myth, Warning, TEXT("  MythLoadWorld <SlotName>           - Load world state"));
    UE_LOG(Myth, Warning, TEXT("  MythListSaves                      - List all save files"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- WEATHER ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListWeather                    - List available weather types"));
    UE_LOG(Myth, Warning, TEXT("  MythSetWeather <Tag>               - Set target weather (transitions)"));
    UE_LOG(Myth, Warning, TEXT("  MythSetWeatherInstant <Tag>        - Set weather instantly"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- TIME OF DAY ---"));
    UE_LOG(Myth, Warning, TEXT("  MythSetTime <Hour>                 - Set time (0-24)"));
    UE_LOG(Myth, Warning, TEXT("  MythAddTime <Hours>                - Add hours"));
    UE_LOG(Myth, Warning, TEXT("  MythPauseTime                      - Pause time"));
    UE_LOG(Myth, Warning, TEXT("  MythResumeTime                     - Resume time"));
    UE_LOG(Myth, Warning, TEXT("  MythSetTimeSpeed <Freq>            - Set update speed (Lower=Faster)"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- ITEMS ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListItems                      - List all item definitions"));
    UE_LOG(Myth, Warning, TEXT("  MythGiveItem <Name> [Count] [Lvl]  - Give item by name (partial match)"));
    UE_LOG(Myth, Warning, TEXT("  MythClearInventory                 - Clear all inventory items"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- ATTRIBUTES ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListAttributes                 - List all attributes and values"));
    UE_LOG(Myth, Warning, TEXT("  MythSetAttribute <Name> <Value>    - Set attribute value"));
    UE_LOG(Myth, Warning, TEXT("  MythSetHealth <Fraction>           - Health to a fraction of max (0-1; above 1 reads as percent)"));
    UE_LOG(Myth, Warning, TEXT("  MythStatus <Name> [0|1]            - Toggle a status tag (Burning, Frozen, ...)"));
    UE_LOG(Myth, Warning, TEXT("  MythClearStatus                    - Clear every status tag"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- PROFICIENCIES ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListProficiencies              - List proficiencies and progress"));
    UE_LOG(Myth, Warning, TEXT("  MythGiveProficiency <Name> <Amt>   - Give proficiency progress"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- SKILLS (server/standalone only) ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListSkills                     - Level, ceiling and practice per skill"));
    UE_LOG(Myth, Warning, TEXT("  MythPracticeSkill <Name> [Count]   - Use a skill Count times, the real levelling route"));
    UE_LOG(Myth, Warning, TEXT("  MythLevelSkill <Name> [Levels]     - Hand over levels without the practice"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- RUNES AND DEEDS (server/standalone only) ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListRunes                      - Name, deed, earned, worn socket, HUD state, timer, rolls"));
    UE_LOG(Myth, Warning, TEXT("  MythGiveRune <Name> [Slot]         - Wear a rune (partial match); -1 = first empty socket"));
    UE_LOG(Myth, Warning, TEXT("  MythRemoveRune <Slot>              - Take the rune out of a socket"));
    UE_LOG(Myth, Warning, TEXT("  MythMoveRune <From> <To>           - Move a worn rune between sockets"));
    UE_LOG(Myth, Warning, TEXT("  MythRuneSlots <N>                  - Set open sockets to N; closing one takes its rune out"));
    UE_LOG(Myth, Warning, TEXT("  MythUnlockAllRunes                 - Earn every deed the rune library gates on"));
    UE_LOG(Myth, Warning, TEXT("  MythUnlockAllRuneSlots             - Open every socket"));
    UE_LOG(Myth, Warning, TEXT("  MythRuneHud                        - Per socket: HUD state, seconds left, stacks"));
    UE_LOG(Myth, Warning, TEXT("  MythRuneRolls                      - Each worn rune's rolled numbers against their ranges"));
    UE_LOG(Myth, Warning, TEXT("  MythGiveDeed <Achievement>         - Earn a deed through its real stat chain"));
    UE_LOG(Myth, Warning, TEXT("  MythClearDeed <Achievement>        - Forget a deed; drops runes that needed it"));
    UE_LOG(Myth, Warning, TEXT("  MythRecordStat <Stat.Tag> [Delta]  - Add to a progression counter"));
    UE_LOG(Myth, Warning, TEXT("  MythSetSeason <Season>             - Spring, Summer, Autumn or Winter"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- LIVING WORLD ---"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldStatus              - System status (thread, fabric, factions, territory)"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldFactions            - List all registered factions"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldTerritory           - Territory info for player's current cell"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldPopulation          - MASS entity counts (NPCs, creatures)"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldSettlements         - List all settlements with faction/density"));
    UE_LOG(Myth, Warning, TEXT("  MythLivingWorldTransferSettlement <ID> <Faction> - Force transfer settlement"));
    UE_LOG(Myth, Warning, TEXT(""));
}


void UMythicCheatManager::MythSettings() {
    /**
     * Runs the REAL route, not a second one.
     *
     * This used to push the settings screen straight onto a layer, which meant the command exercised a
     * path no player ever takes - and hid the very bug it was meant to catch, because pushing to the
     * layer is exactly what broke pause and the cursor. It now asks the live escape menu to open settings
     * the way its own button does, so a green result here means the real thing works.
     */
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    for (TObjectIterator<UMythicEscapeMenuWidget> It; It; ++It) {
        UMythicEscapeMenuWidget *Menu = *It;
        if (!Menu || Menu->HasAnyFlags(RF_ClassDefaultObject) || Menu->GetWorld() != PC->GetWorld()) {
            continue;
        }
        if (Menu->IsActivated()) {
            Menu->RunAction(EMythicEscapeAction::Settings);
            return;
        }
    }

    UE_LOG(Myth, Warning,
           TEXT("MythSettings: no active escape menu. Press Escape first - settings opens inside it."));
}

void UMythicCheatManager::MythSaveCharacter(const FString &SlotName) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicSaveGameSubsystem *SaveSys = PC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>();
    if (!SaveSys || !PC->PlayerState) {
        return;
    }

    SaveSys->SaveCharacter(PC->PlayerState, SlotName);
    UE_LOG(MythSaveLoad, Warning, TEXT(">>> Started saving character to '%s'... Check log for result."), *SlotName);
}

void UMythicCheatManager::MythLoadCharacter(const FString &SlotName) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicSaveGameSubsystem *SaveSys = PC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>();
    if (!SaveSys || !PC->PlayerState) {
        return;
    }

    SaveSys->LoadCharacter(PC->PlayerState, SlotName);
    UE_LOG(MythSaveLoad, Warning, TEXT(">>> Started loading character from '%s'... Check log for result."), *SlotName);
}

void UMythicCheatManager::MythSaveWorld(const FString &SlotName) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicSaveGameSubsystem *SaveSys = PC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>();
    if (!SaveSys) {
        return;
    }

    SaveSys->SaveWorld(SlotName);
    UE_LOG(MythSaveLoad, Warning, TEXT(">>> Started saving world to '%s'... Check log for result."), *SlotName);
}

void UMythicCheatManager::MythLoadWorld(const FString &SlotName) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicSaveGameSubsystem *SaveSys = PC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>();
    if (!SaveSys) {
        return;
    }

    SaveSys->LoadWorld(SlotName);
    UE_LOG(MythSaveLoad, Warning, TEXT(">>> Started loading world from '%s'... Check log for result."), *SlotName);
}

void UMythicCheatManager::MythListSaves() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicSaveGameSubsystem *SaveSys = PC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>();
    if (!SaveSys) {
        return;
    }

    TArray<FString> SaveFiles = SaveSys->GetLocalSaveFiles();

    UE_LOG(MythSaveLoad, Warning, TEXT(">>> Save files (%d):"), SaveFiles.Num());
    for (const FString &File : SaveFiles) {
        UE_LOG(MythSaveLoad, Warning, TEXT("    - %s"), *File);
    }
    if (SaveFiles.Num() == 0) {
        UE_LOG(MythSaveLoad, Warning, TEXT("    (none)"));
    }
}


void UMythicCheatManager::MythListWeather() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (!EnvSys) {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Subsystem"));
        return;
    }

    auto Controller = EnvSys->GetEnvironmentController();
    if (!Controller) {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
        return;
    }

    FGameplayTag CurrentWeather = EnvSys->GetWeather();
    UE_LOG(Myth, Warning, TEXT(">>> Current Weather: %s"), *CurrentWeather.ToString());
    UE_LOG(Myth, Warning, TEXT(">>> Available Weather Types (%d):"), Controller->GetWeatherTypes().Num());

    for (const auto &Type : Controller->GetWeatherTypes()) {
        if (Type) {
            UE_LOG(Myth, Warning, TEXT("    - %s"), *Type->Tag.ToString());
        }
    }
}

void UMythicCheatManager::MythSetWeather(const FString &WeatherTag) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (!EnvSys) {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Subsystem"));
        return;
    }

    FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*WeatherTag), false);
    if (!Tag.IsValid()) {
        UE_LOG(Myth, Error, TEXT(">>> Invalid tag '%s'. Use ListWeather."), *WeatherTag);
        return;
    }

    EnvSys->SetTargetWeather(Tag);
    UE_LOG(Myth, Warning, TEXT(">>> Weather target set to '%s'"), *WeatherTag);
}

void UMythicCheatManager::MythSetWeatherInstant(const FString &WeatherTag) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (!EnvSys) {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Subsystem"));
        return;
    }

    FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*WeatherTag), false);
    if (!Tag.IsValid()) {
        UE_LOG(Myth, Error, TEXT(">>> Invalid tag '%s'. Use ListWeather."), *WeatherTag);
        return;
    }

    EnvSys->SetWeatherInstantly(Tag);
    UE_LOG(Myth, Warning, TEXT(">>> Weather set instantly to '%s'"), *WeatherTag);
}


void UMythicCheatManager::MythSetTime(float Hour) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (auto Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr) {
        FDateTime Current = Controller->GetDateTime();
        FDateTime NewTime = FDateTime(Current.GetYear(), Current.GetMonth(), Current.GetDay(), FMath::FloorToInt(Hour),
                                      FMath::FloorToInt((Hour - FMath::FloorToInt(Hour)) * 60));

        Controller->SetTime(NewTime);
        UE_LOG(Myth, Warning, TEXT(">>> Time set to %s"), *NewTime.ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
    }
}

void UMythicCheatManager::MythAddTime(float Hours) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (auto Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr) {
        Controller->AddTime(FTimespan::FromHours(Hours));
        UE_LOG(Myth, Warning, TEXT(">>> Added %.2f hours. New Time: %s"), Hours, *Controller->GetDateTime().ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
    }
}

void UMythicCheatManager::MythPauseTime() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (auto Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr) {
        Controller->PauseTime();
        UE_LOG(Myth, Warning, TEXT(">>> Time Paused"));
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
    }
}

void UMythicCheatManager::MythResumeTime() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (auto Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr) {
        Controller->ResumeTime();
        UE_LOG(Myth, Warning, TEXT(">>> Time Resumed"));
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
    }
}

void UMythicCheatManager::MythSetTimeSpeed(float NewFrequency) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (auto Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr) {
        Controller->SetTimeUpdateFrequency(NewFrequency);
        UE_LOG(Myth, Warning, TEXT(">>> Time Update Frequency set to %.4f"), NewFrequency);
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
    }
}


void UMythicCheatManager::MythListItems() {
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    TArray<FPrimaryAssetId> ItemAssetIds;
    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ItemDefinitionType, ItemAssetIds);

    UE_LOG(Myth, Warning, TEXT(">>> Items (%d):"), ItemAssetIds.Num());
    for (const FPrimaryAssetId &AssetId : ItemAssetIds) {
        UE_LOG(Myth, Warning, TEXT("    - %s"), *AssetId.PrimaryAssetName.ToString());
    }
}

void UMythicCheatManager::MythGiveItem(const FString &ItemName, int32 Count, int32 ItemLevel) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UMythicLootManagerSubsystem *LootManager = PC->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    if (!LootManager) {
        UE_LOG(Myth, Error, TEXT(">>> No LootManager Subsystem"));
        return;
    }

    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    TArray<FPrimaryAssetId> ItemAssetIds;
    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ItemDefinitionType, ItemAssetIds);

    UItemDefinition *MatchedItem = nullptr;
    for (int32 Pass = 0; Pass < 2 && !MatchedItem; ++Pass) {
        for (const FPrimaryAssetId &AssetId : ItemAssetIds) {
            const FString Name = AssetId.PrimaryAssetName.ToString();
            const bool bMatch = (Pass == 0) ? Name.Equals(ItemName, ESearchCase::IgnoreCase)
                                            : Name.Contains(ItemName, ESearchCase::IgnoreCase);
            if (bMatch) {
                FSoftObjectPath ItemPath = AssetManager.GetPrimaryAssetPath(AssetId);
                MatchedItem = Cast<UItemDefinition>(ItemPath.TryLoad());
                if (MatchedItem) {
                    break;
                }
            }
        }
    }

    if (!MatchedItem) {
        UE_LOG(Myth, Error, TEXT(">>> No item found matching '%s'. Use ListItems."), *ItemName);
        return;
    }

    AMythicWorldItem *Dropped = LootManager->CreateAndGive(
        MatchedItem,
        FMath::Max(1, Count),
        PC,
        PC,
        FMath::Max(1, ItemLevel)
        );

    if (Dropped) {
        UE_LOG(Myth, Warning, TEXT(">>> Gave %d x %s (dropped)"), Count, *MatchedItem->Name.ToString());
    }
    else {
        UE_LOG(Myth, Warning, TEXT(">>> Gave %d x %s"), Count, *MatchedItem->Name.ToString());
    }
}

namespace {
TArray<UMythicSkillDefinition *> Cheat_AllSkills() {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UMythicSkillDefinition::StaticClass()->GetClassPathName(), Assets);

    TArray<UMythicSkillDefinition *> Out;
    for (const FAssetData &Asset : Assets) {
        if (UMythicSkillDefinition *Skill = Cast<UMythicSkillDefinition>(Asset.GetAsset())) {
            Out.Add(Skill);
        }
    }
    Out.Sort([](const UMythicSkillDefinition &A, const UMythicSkillDefinition &B) { return A.GetName() < B.GetName(); });
    return Out;
}

// Every skill verb decides on the server, so a cheat run from a client would look like it worked and change nothing.
UMythicSkillComponent *Cheat_AuthoritativeSkills(APlayerController *PC) {
    AMythicPlayerState *PS = PC ? Cast<AMythicPlayerState>(PC->PlayerState) : nullptr;
    if (!PS) {
        UE_LOG(Myth, Error, TEXT(">>> No Mythic player state"));
        return nullptr;
    }
    if (!PS->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT(">>> Skill cheats decide on the server. Run this on the host or in standalone."));
        return nullptr;
    }
    return PS->GetSkillComponent();
}

UMythicSkillDefinition *Cheat_FindSkill(const FString &SkillName) {
    for (UMythicSkillDefinition *Skill : Cheat_AllSkills()) {
        if (Skill->GetName().Contains(SkillName) || Skill->Name.ToString().Contains(SkillName)) {
            return Skill;
        }
    }
    UE_LOG(Myth, Error, TEXT(">>> No skill matching '%s'. MythListSkills for the list."), *SkillName);
    return nullptr;
}
}

void UMythicCheatManager::MythListSkills() {
    UMythicSkillComponent *Skills = Cheat_AuthoritativeSkills(GetOuterAPlayerController());
    const TArray<UMythicSkillDefinition *> All = Cheat_AllSkills();
    UE_LOG(Myth, Warning, TEXT(">>> %d skills, %d modifiers carried at once"), All.Num(),
           Skills ? Skills->GetModifierCapacity() : 0);
    for (UMythicSkillDefinition *Skill : All) {
        UE_LOG(Myth, Warning, TEXT("  %-26s lvl %d/%d  %d uses (next at %d)  %d modifiers"), *Skill->GetName(),
               Skills ? Skills->GetSkillLevel(Skill) : 0, Skills ? Skills->GetMaxSkillLevel(Skill) : 0,
               Skills ? Skills->GetSkillUses(Skill) : 0, Skills ? Skills->GetUsesForNextLevel(Skill) : 0,
               Skill->Modifiers.Num());
    }
}

void UMythicCheatManager::MythPracticeSkill(const FString &SkillName, int32 Count) {
    UMythicSkillComponent *Skills = Cheat_AuthoritativeSkills(GetOuterAPlayerController());
    UMythicSkillDefinition *Skill = Skills ? Cheat_FindSkill(SkillName) : nullptr;
    if (!Skill) {
        return;
    }
    for (int32 i = 0; i < FMath::Max(Count, 1); i++) {
        Skills->RecordSkillUse(Skill);
    }
    UE_LOG(Myth, Warning, TEXT(">>> %s: %d uses, level %d/%d"), *Skill->GetName(), Skills->GetSkillUses(Skill),
           Skills->GetSkillLevel(Skill), Skills->GetMaxSkillLevel(Skill));
}

void UMythicCheatManager::MythLevelSkill(const FString &SkillName, int32 Levels) {
    UMythicSkillComponent *Skills = Cheat_AuthoritativeSkills(GetOuterAPlayerController());
    UMythicSkillDefinition *Skill = Skills ? Cheat_FindSkill(SkillName) : nullptr;
    if (!Skill) {
        return;
    }
    Skills->GrantSkillLevel(Skill, FMath::Max(Levels, 1));
    UE_LOG(Myth, Warning, TEXT(">>> %s: level %d/%d, %d points to spend"), *Skill->GetName(), Skills->GetSkillLevel(Skill),
           Skills->GetMaxSkillLevel(Skill), Skills->GetAvailablePoints(Skill));
}

void UMythicCheatManager::MythClearInventory() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    TArray<UMythicInventoryComponent *> Inventories;
    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
        Inventories = InvProvider->GetAllInventoryComponents();
    }

    if (Inventories.Num() == 0) {
        UE_LOG(Myth, Error, TEXT(">>> No inventories found"));
        return;
    }

    int32 TotalCleared = 0;
    for (UMythicInventoryComponent *Inv : Inventories) {
        if (!Inv) {
            continue;
        }
        for (int32 i = 0; i < Inv->GetNumSlots(); ++i) {
            if (Inv->GetItem(i)) {
                TotalCleared++;
                Inv->SetItemInSlot(i, nullptr);
            }
        }
    }

    UE_LOG(Myth, Warning, TEXT(">>> Cleared %d items"), TotalCleared);
}


void UMythicCheatManager::MythListAttributes() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState);
    if (!PS) {
        UE_LOG(Myth, Error, TEXT(">>> No MythicPlayerState"));
        return;
    }

    UMythicAbilitySystemComponent *ASC = Cast<UMythicAbilitySystemComponent>(PS->GetAbilitySystemComponent());
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }

    const TArray<UMythicAttributeSet *> &AttributeSets = ASC->GetAttributeSets();

    UE_LOG(Myth, Warning, TEXT(">>> Attributes:"));

    for (const UMythicAttributeSet *AttrSet : AttributeSets) {
        if (!AttrSet) {
            continue;
        }

        UE_LOG(Myth, Warning, TEXT("  [%s]"), *AttrSet->GetClass()->GetName());

        for (TFieldIterator<FProperty> PropIt(AttrSet->GetClass()); PropIt; ++PropIt) {
            FProperty *Property = *PropIt;
            if (FStructProperty *StructProp = CastField<FStructProperty>(Property)) {
                if (StructProp->Struct == FGameplayAttributeData::StaticStruct()) {
                    const FGameplayAttributeData *AttrData = StructProp->ContainerPtrToValuePtr<FGameplayAttributeData>(AttrSet);
                    if (AttrData) {
                        UE_LOG(Myth, Warning, TEXT("    %s: %.2f"), *Property->GetName(), AttrData->GetCurrentValue());
                    }
                }
            }
        }
    }
}

namespace {
UMythicAbilitySystemComponent *Cheat_PlayerASC(APlayerController *PC) {
    AMythicPlayerState *PS = PC ? Cast<AMythicPlayerState>(PC->PlayerState) : nullptr;
    return PS ? Cast<UMythicAbilitySystemComponent>(PS->GetAbilitySystemComponent()) : nullptr;
}

UMythicStatusRegistry *Cheat_StatusRegistry(const APlayerController *PC) {
    const UWorld *World = PC ? PC->GetWorld() : nullptr;
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
}

FString Cheat_StatusShortName(const UMythicStatusEffectDefinition *Definition) {
    const FString Full = Definition->StatusType.ToString();
    int32 Dot = INDEX_NONE;
    return Full.FindLastChar(TCHAR('.'), Dot) ? Full.RightChop(Dot + 1) : Full;
}

UMythicStatusEffectDefinition *Cheat_FindStatus(const APlayerController *PC, const FString &Name) {
    const UMythicStatusRegistry *Registry = Cheat_StatusRegistry(PC);
    if (!Registry) {
        UE_LOG(Myth, Error, TEXT(">>> No status registry"));
        return nullptr;
    }
    for (UMythicStatusEffectDefinition *Definition : Registry->GetAllStatuses()) {
        if (Definition && Cheat_StatusShortName(Definition).Equals(Name, ESearchCase::IgnoreCase)) {
            return Definition;
        }
    }
    UE_LOG(Myth, Error, TEXT(">>> Unknown status '%s'. Known:"), *Name);
    for (const UMythicStatusEffectDefinition *Definition : Registry->GetAllStatuses()) {
        if (Definition) {
            UE_LOG(Myth, Warning, TEXT("      %s"), *Cheat_StatusShortName(Definition));
        }
    }
    return nullptr;
}

// Buildup has to arrive as a real effect. SetNumericAttributeBase skips PostGameplayEffectExecute, which is
// where the threshold, reactions, cues and CC escalation live, so a status could never trigger from it.
void Cheat_AddBuildup(UMythicAbilitySystemComponent *ASC, const FGameplayAttribute &Attribute, float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Cheat_StatusBuildup")));
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = Attribute;
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}
}

void UMythicCheatManager::MythStatus(const FString &StatusName, int32 bOn) {
    APlayerController *PC = GetOuterAPlayerController();
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(PC);
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }
    const UMythicStatusEffectDefinition *Definition = Cheat_FindStatus(PC, StatusName);
    if (!Definition) {
        return;
    }

    if (bOn == 0) {
        if (Definition->GrantedStateTag.IsValid()) {
            ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Definition->GrantedStateTag));
        }
        UE_LOG(Myth, Warning, TEXT(">>> %s removed"), *Cheat_StatusShortName(Definition));
        return;
    }

    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn || !Pawn->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT(">>> MythStatus needs authority - run it on the server or in standalone"));
        return;
    }

    if (UMythicStatusRegistry::ApplyStatusToActor(Pawn, Definition->StatusType, Pawn)) {
        UE_LOG(Myth, Warning, TEXT(">>> %s applied directly. This skips buildup, immunity and diminishing returns - use MythBuildup for the real path"),
               *Cheat_StatusShortName(Definition));
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> %s has no EffectToApply authored"), *Cheat_StatusShortName(Definition));
    }
}

void UMythicCheatManager::MythClearStatus() {
    APlayerController *PC = GetOuterAPlayerController();
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(PC);
    const UMythicStatusRegistry *Registry = Cheat_StatusRegistry(PC);
    if (!ASC || !Registry) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC or status registry"));
        return;
    }
    for (const UMythicStatusEffectDefinition *Definition : Registry->GetAllStatuses()) {
        if (!Definition) {
            continue;
        }
        if (Definition->GrantedStateTag.IsValid()) {
            ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Definition->GrantedStateTag));
            ASC->SetLooseGameplayTagCount(Definition->GrantedStateTag, 0);
        }
        if (Definition->BuildupAttribute.IsValid()) {
            ASC->SetNumericAttributeBase(Definition->BuildupAttribute, 0.0f);
        }
    }
    UE_LOG(Myth, Warning, TEXT(">>> Statuses and buildup cleared"));
}

void UMythicCheatManager::MythBuildup(const FString &StatusName, float Amount) {
    APlayerController *PC = GetOuterAPlayerController();
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(PC);
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }
    const UMythicStatusEffectDefinition *Definition = Cheat_FindStatus(PC, StatusName);
    if (!Definition) {
        return;
    }
    if (!Definition->BuildupAttribute.IsValid()) {
        UE_LOG(Myth, Error, TEXT(">>> %s has no BuildupAttribute authored"), *Cheat_StatusShortName(Definition));
        return;
    }

    Cheat_AddBuildup(ASC, Definition->BuildupAttribute, Amount);

    const float Now = ASC->GetNumericAttribute(Definition->BuildupAttribute);
    UE_LOG(Myth, Warning, TEXT(">>> %s buildup +%.1f -> %.1f of %.1f"), *Cheat_StatusShortName(Definition), Amount, Now,
           UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f));
}

void UMythicCheatManager::MythProcChance(const FString &StatusName, float Chance) {
    APlayerController *PC = GetOuterAPlayerController();
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(PC);
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }
    const UMythicStatusEffectDefinition *Definition = Cheat_FindStatus(PC, StatusName);
    if (!Definition) {
        return;
    }
    const FString PropertyName = FString::Printf(TEXT("Apply%sOnHitChance"), *Cheat_StatusShortName(Definition));
    FProperty *Property = FindFProperty<FProperty>(UMythicAttributeSet_Offense::StaticClass(), FName(*PropertyName));
    if (!Property) {
        UE_LOG(Myth, Error, TEXT(">>> No attribute named %s"), *PropertyName);
        return;
    }
    ASC->SetNumericAttributeBase(FGameplayAttribute(Property), Chance);
    UE_LOG(Myth, Warning, TEXT(">>> %s = %.2f - hit something to roll it"), *PropertyName, Chance);
}

void UMythicCheatManager::MythStatusList() {
    APlayerController *PC = GetOuterAPlayerController();
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(PC);
    const UMythicStatusRegistry *Registry = Cheat_StatusRegistry(PC);
    if (!ASC || !Registry) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC or status registry"));
        return;
    }
    UE_LOG(Myth, Warning, TEXT(">>> %-10s %-24s %8s %10s %7s %s"), TEXT("STATUS"), TEXT("GRANTED TAG"), TEXT("BUILDUP"),
           TEXT("THRESHOLD"), TEXT("RESIST"), TEXT("ACTIVE"));
    for (const UMythicStatusEffectDefinition *Definition : Registry->GetAllStatuses()) {
        if (!Definition) {
            continue;
        }
        const float Buildup = Definition->BuildupAttribute.IsValid() ? ASC->GetNumericAttribute(Definition->BuildupAttribute) : -1.0f;
        const float Resist = ASC->HasAttributeSetForAttribute(Definition->ResistanceAttribute)
                                 ? ASC->GetNumericAttribute(Definition->ResistanceAttribute)
                                 : 0.0f;
        const bool bActive = Definition->GrantedStateTag.IsValid() && ASC->HasMatchingGameplayTag(Definition->GrantedStateTag);
        UE_LOG(Myth, Warning, TEXT("    %-10s %-24s %8.1f %10.1f %7.2f %s"), *Cheat_StatusShortName(Definition),
               *Definition->GrantedStateTag.ToString(), Buildup, UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f),
               Resist, bActive ? TEXT("yes") : TEXT("no"));
    }
}

void UMythicCheatManager::MythSetAttribute(const FString &AttributeName, float Value) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState);
    if (!PS) {
        UE_LOG(Myth, Error, TEXT(">>> No MythicPlayerState"));
        return;
    }

    UMythicAbilitySystemComponent *ASC = Cast<UMythicAbilitySystemComponent>(PS->GetAbilitySystemComponent());
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }

    const TArray<UMythicAttributeSet *> &AttributeSets = ASC->GetAttributeSets();

    for (int32 Pass = 0; Pass < 2; ++Pass) {
        for (const UMythicAttributeSet *AttrSet : AttributeSets) {
            if (!AttrSet) {
                continue;
            }

            for (TFieldIterator<FProperty> PropIt(AttrSet->GetClass()); PropIt; ++PropIt) {
                FProperty *Property = *PropIt;
                const bool bMatch = (Pass == 0) ? Property->GetName().Equals(AttributeName, ESearchCase::IgnoreCase)
                                                : Property->GetName().Contains(AttributeName, ESearchCase::IgnoreCase);
                if (bMatch) {
                    if (FStructProperty *StructProp = CastField<FStructProperty>(Property)) {
                        if (StructProp->Struct == FGameplayAttributeData::StaticStruct()) {
                            FGameplayAttribute Attribute(Property);
                            ASC->SetNumericAttributeBase(Attribute, Value);
                            UE_LOG(Myth, Warning, TEXT(">>> Set %s to %.2f"), *Property->GetName(), Value);
                            return;
                        }
                    }
                }
            }
        }
    }

    UE_LOG(Myth, Error, TEXT(">>> Attribute '%s' not found. Use ListAttributes."), *AttributeName);
}


void UMythicCheatManager::MythListProficiencies() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UProficiencyComponent *ProfComp = nullptr;
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent());
    }
    if (!ProfComp || ProfComp->Proficiencies.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT(">>> No proficiencies configured"));
        return;
    }

    AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState);
    UAbilitySystemComponent *ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;

    UE_LOG(Myth, Warning, TEXT(">>> Proficiencies (%d):"), ProfComp->Proficiencies.Num());

    for (const FProficiency &Prof : ProfComp->Proficiencies) {
        FString Name = Prof.Definition ? Prof.Definition->GetName() : TEXT("Unknown");

        float CurrentProgress = 0.0f;
        const FGameplayAttribute ProgressAttribute = Prof.GetProgressAttribute();
        if (ASC && ProgressAttribute.IsValid()) {
            bool bFound = false;
            CurrentProgress = ASC->GetGameplayAttributeValue(ProgressAttribute, bFound);
        }

        UE_LOG(Myth, Warning, TEXT("    - %s: %.0f progress"), *Name, CurrentProgress);
    }
}

void UMythicCheatManager::MythGiveProficiency(const FString &ProficiencyName, float Amount) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }

    UProficiencyComponent *ProfComp = nullptr;
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        ProfComp = const_cast<UProficiencyComponent*>(MythicPC->GetProficiencyComponent());
    }
    if (!ProfComp || ProfComp->Proficiencies.Num() == 0) {
        UE_LOG(Myth, Error, TEXT(">>> No proficiencies available"));
        return;
    }

    AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState);
    if (!PS) {
        UE_LOG(Myth, Error, TEXT(">>> No MythicPlayerState"));
        return;
    }

    UAbilitySystemComponent *ASC = PS->GetAbilitySystemComponent();
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }

    for (FProficiency &Prof : ProfComp->Proficiencies) {
        FString Name = Prof.Definition ? Prof.Definition->GetName() : TEXT("");
        if (Name.Contains(ProficiencyName, ESearchCase::IgnoreCase)) {
            const FGameplayAttribute ProgressAttribute = Prof.GetProgressAttribute();
            if (!ProgressAttribute.IsValid()) {
                UE_LOG(Myth, Error, TEXT(">>> Proficiency '%s' has no canonical Progress Stat"), *Name);
                return;
            }

            bool bFound = false;
            float Current = ASC->GetGameplayAttributeValue(ProgressAttribute, bFound);
            if (bFound) {
                float NewVal = Current + Amount;
                ASC->SetNumericAttributeBase(ProgressAttribute, NewVal);
                UE_LOG(Myth, Warning, TEXT(">>> %s: %.0f -> %.0f (+%.0f)"), *Name, Current, NewVal, Amount);
                return;
            }
        }
    }

    UE_LOG(Myth, Error, TEXT(">>> Proficiency '%s' not found. Use ListProficiencies."), *ProficiencyName);
}


void UMythicCheatManager::MythLivingWorldStatus() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW) {
        UE_LOG(Myth, Error, TEXT(">>> Living World Subsystem not found"));
        return;
    }

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== LIVING WORLD STATUS ==="));
    UE_LOG(Myth, Warning, TEXT("  System active: %s"), LW->IsSystemActive() ? TEXT("YES") : TEXT("NO"));

    if (const UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
        UE_LOG(Myth, Warning, TEXT("  Causal Fabric: initialized (capacity %d)"), Fabric->GetCapacity());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  Causal Fabric: NOT initialized"));
    }

    if (const UMythicFactionDatabase *FDB = LW->GetFactionDatabase()) {
        UE_LOG(Myth, Warning, TEXT("  Faction DB: %d active / %d max"), FDB->GetActiveFactionCount(), FDB->GetMaxFactions());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  Faction DB: NOT initialized"));
    }

    if (const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid()) {
        UE_LOG(Myth, Warning, TEXT("  Territory Grid: initialized"));
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  Territory Grid: NOT initialized"));
    }

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldFactions() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetFactionDatabase()) {
        UE_LOG(Myth, Error, TEXT(">>> Living World / Faction DB not available"));
        return;
    }

    const UMythicFactionDatabase *FDB = LW->GetFactionDatabase();
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== FACTIONS (%d active / %d max) ==="), FDB->GetActiveFactionCount(), FDB->GetMaxFactions());

    FDB->ForEachAliveFaction([](FMythicFactionId Id, const FMythicFactionData &Data) {
        FString Flags;
        if (Data.bControlsTerritory) {
            Flags += TEXT("T");
        }
        if (Data.bHasEconomy) {
            Flags += TEXT("E");
        }
        if (Data.bHasCivilianPopulation) {
            Flags += TEXT("C");
        }
        if (Data.bParticipatesInTrade) {
            Flags += TEXT("$");
        }
        if (Data.bCanNegotiate) {
            Flags += TEXT("D");
        }

        UE_LOG(Myth, Warning, TEXT("  [%d] %s (%s) [%s] | Mil: %.2f | Pop: %d | Cells: %d"),
               Id.Index,
               *Data.DisplayName.ToString(),
               *Data.FactionTag.ToString(),
               *Flags,
               Data.MilitaryStrength,
               Data.Population,
               Data.ControlledCellCount);

        UE_LOG(Myth, Warning, TEXT("       Reserves: F=%.1f M=%.1f A=%.1f W=%.1f | Prices: F=%.2f M=%.2f A=%.2f W=%.2f"),
               Data.Reserves.Food, Data.Reserves.Materials, Data.Reserves.Arms, Data.Reserves.Wealth,
               Data.Prices.Food, Data.Prices.Materials, Data.Prices.Arms, Data.Prices.Wealth);
    });

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldTerritory() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) {
        UE_LOG(Myth, Error, TEXT(">>> No pawn"));
        return;
    }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetTerritoryGrid()) {
        UE_LOG(Myth, Error, TEXT(">>> Living World / Territory Grid not available"));
        return;
    }

    const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid();
    const FVector PlayerPos = PC->GetPawn()->GetActorLocation();
    const FMythicCellCoord Cell = Grid->WorldToCell(PlayerPos);
    const FMythicTerritoryCell CellData = Grid->GetCell(Cell);

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== TERRITORY (Player Cell) ==="));
    UE_LOG(Myth, Warning, TEXT("  Position: (%.0f, %.0f, %.0f)"), PlayerPos.X, PlayerPos.Y, PlayerPos.Z);
    UE_LOG(Myth, Warning, TEXT("  Cell: (%d, %d)"), Cell.X, Cell.Y);
    UE_LOG(Myth, Warning, TEXT("  Dominant Faction Index: %d"), CellData.DominantFaction.Index);
    UE_LOG(Myth, Warning, TEXT("  Influence: %.3f"), CellData.Influence);
    UE_LOG(Myth, Warning, TEXT("  Player Owned: %s (Player %d)"), CellData.bPlayerOwned ? TEXT("YES") : TEXT("NO"), CellData.OwningPlayerIndex);

    if (CellData.DominantFaction.IsValid()) {
        if (const UMythicFactionDatabase *FDB = LW->GetFactionDatabase()) {
            FMythicFactionData FactionData;
            if (FDB->GetFaction(CellData.DominantFaction, FactionData)) {
                UE_LOG(Myth, Warning, TEXT("  Dominant Faction: %s"), *FactionData.DisplayName.ToString());
            }
        }
    }
    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldPopulation() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UWorld *World = PC->GetWorld();
    if (!World) {
        UE_LOG(Myth, Error, TEXT(">>> No World"));
        return;
    }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    if (!MassSubsystem) {
        UE_LOG(Myth, Error, TEXT(">>> MASS Entity Subsystem not found"));
        return;
    }

    TSharedPtr<FMassEntityManager> EntityManagerPtr = TSharedPtr<FMassEntityManager>(&MassSubsystem->GetMutableEntityManager(), [](FMassEntityManager *) {});

    int32 NPCCount = 0;
    {
        FMassEntityQuery NPCQuery(EntityManagerPtr);
        NPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
        NPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        NPCCount = NPCQuery.GetNumMatchingEntities();
    }

    int32 CreatureCount = 0;
    {
        FMassEntityQuery CreatureQuery(EntityManagerPtr);
        CreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
        CreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
        CreatureCount = CreatureQuery.GetNumMatchingEntities();
    }

    int32 HydratedCount = 0;
    {
        FMassEntityQuery HydratedQuery(EntityManagerPtr);
        HydratedQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
        HydratedQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        HydratedCount = HydratedQuery.GetNumMatchingEntities();
    }

    const int32 TotalCount = NPCCount + CreatureCount;

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== LIVING WORLD POPULATION ==="));
    UE_LOG(Myth, Warning, TEXT("  Total MASS entities: %d"), TotalCount);
    UE_LOG(Myth, Warning, TEXT("    NPCs: %d"), NPCCount);
    UE_LOG(Myth, Warning, TEXT("    Creatures: %d"), CreatureCount);
    UE_LOG(Myth, Warning, TEXT("    Hydrated (Tier 1+): %d"), HydratedCount);
    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldSettlements() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW) {
        UE_LOG(Myth, Error, TEXT(">>> Living World Subsystem not found"));
        return;
    }

    UMythicSettlementRegistry *Registry = LW->GetSettlementRegistry();
    if (!Registry) {
        UE_LOG(Myth, Error, TEXT(">>> Settlement Registry not available"));
        return;
    }

    const UMythicFactionDatabase *FDB = LW->GetFactionDatabase();

    TArray<int32> SettlementIds;
    Registry->GetAllSettlementIds(SettlementIds);

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== SETTLEMENTS (%d) ==="), Registry->GetSettlementCount());

    for (const int32 Id : SettlementIds) {
        FMythicSettlementData Data;
        if (!LW->CopySettlementById(Id, Data)) {
            continue;
        }

        FString FactionName = TEXT("Unknown");
        if (FDB && Data.GoverningFaction.IsValid()) {
            FMythicFactionData FactionData;
            if (FDB->GetFaction(Data.GoverningFaction, FactionData)) {
                FactionName = FactionData.DisplayName.ToString();
            }
        }

        UE_LOG(Myth, Warning, TEXT("  [ID=%d] %s%s"),
               Id,
               *Data.DisplayName.ToString(),
               Data.bIsCapital ? TEXT(" (CAPITAL)") : TEXT(""));
        UE_LOG(Myth, Warning, TEXT("       Faction: %s (ID=%d) | MaxDensity: %d/cell | Cells: %d"),
               *FactionName,
               Data.GoverningFaction.Index,
               Data.MaxPopulationDensity,
               Data.RasterizedCells.Num());
    }

    if (SettlementIds.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("  (no settlements registered)"));
    }

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldTransferSettlement(int32 SettlementId, int32 FactionIndex) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW) {
        UE_LOG(Myth, Error, TEXT(">>> Living World Subsystem not found"));
        return;
    }

    UMythicSettlementRegistry *Registry = LW->GetSettlementRegistry();
    UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid();
    UMythicFactionDatabase *FDB = LW->GetFactionDatabase();

    if (!Registry || !Grid || !FDB) {
        UE_LOG(Myth, Error, TEXT(">>> Missing required subsystems (Registry, Grid, or FactionDB)"));
        return;
    }

    FMythicSettlementData Data;
    if (!LW->CopySettlementById(SettlementId, Data)) {
        UE_LOG(Myth, Error, TEXT(">>> Settlement ID %d not found. Use MythLivingWorldSettlements."), SettlementId);
        return;
    }

    FMythicFactionId NewFaction;
    NewFaction.Index = FactionIndex;
    FMythicFactionData NewFactionData;
    if (!FDB->GetFaction(NewFaction, NewFactionData)) {
        UE_LOG(Myth, Error, TEXT(">>> Faction index %d not found. Use MythLivingWorldFactions."), FactionIndex);
        return;
    }

    FMythicFactionData OldFactionData;
    const FString OldFactionName = FDB->GetFaction(Data.GoverningFaction, OldFactionData)
        ? OldFactionData.DisplayName.ToString()
        : TEXT("Unknown");
    const FString NewFactionName = NewFactionData.DisplayName.ToString();

    LW->TransferSettlement(SettlementId, NewFaction);

    UE_LOG(Myth, Warning, TEXT(">>> Settlement '%s' (ID=%d) transferred: %s -> %s"),
           *Data.DisplayName.ToString(),
           SettlementId,
           *OldFactionName,
           *NewFactionName);
}

void UMythicCheatManager::MythToggleLivingWorldDebug() {
    AMythicLivingWorldDebugActor *ExistingDebugActor = nullptr;
    for (TActorIterator<AMythicLivingWorldDebugActor> It(GetWorld()); It; ++It) {
        ExistingDebugActor = *It;
        break;
    }

    if (ExistingDebugActor) {
        ExistingDebugActor->Destroy();
        UE_LOG(Myth, Display, TEXT("Living World Debug Visualization DISABLED"));
    }
    else {
        GetWorld()->SpawnActor<AMythicLivingWorldDebugActor>();
        UE_LOG(Myth, Display, TEXT("Living World Debug Visualization ENABLED"));
    }
}


void UMythicCheatManager::MythLivingWorldSimulateEvent(const FString &ActionTag, const FString &MoralAxis, float MoralValue) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) {
        UE_LOG(Myth, Error, TEXT(">>> No pawn to simulate event from"));
        return;
    }

    UWorld *World = PC->GetWorld();
    if (!World) {
        return;
    }

    UMythicActionEventSubsystem *ActionSub = World->GetSubsystem<UMythicActionEventSubsystem>();
    if (!ActionSub) {
        UE_LOG(Myth, Error, TEXT(">>> ActionEventSubsystem not available (server only)"));
        return;
    }

    int32 AxisIndex = -1;
    static const FString AxisNames[] = {
        TEXT("Violence"), TEXT("Theft"), TEXT("Deception"), TEXT("Mercy"),
        TEXT("Loyalty"), TEXT("Sanctity"), TEXT("Authority"), TEXT("Arcane")
    };
    for (int32 i = 0; i < MoralAxisCount; ++i) {
        if (AxisNames[i].Contains(MoralAxis, ESearchCase::IgnoreCase)) {
            AxisIndex = i;
            break;
        }
    }

    if (AxisIndex < 0) {
        UE_LOG(Myth, Error, TEXT(">>> Unknown moral axis '%s'. Valid: Violence, Theft, Deception, Mercy, Loyalty, Sanctity, Authority, Arcane"), *MoralAxis);
        return;
    }

    FMythicActionEvent Event;
    Event.Perpetrator = PC->GetPawn();
    Event.ActionTag = FGameplayTag::RequestGameplayTag(FName(*ActionTag), false);
    if (!Event.ActionTag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT(">>> Tag '%s' not registered — event will still submit with invalid tag"), *ActionTag);
    }
    Event.MoralVector.AxisValues[AxisIndex] = MoralValue;
    Event.CategoryFlags = 0x01;
    Event.Significance = FMath::Abs(MoralValue);

    ActionSub->SubmitAction(Event);

    UE_LOG(Myth, Warning, TEXT(">>> Simulated event: Tag=%s Axis=%s Value=%.2f Significance=%.2f"),
           *ActionTag, *AxisNames[AxisIndex], MoralValue, Event.Significance);
}

void UMythicCheatManager::MythLivingWorldPressure() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) {
        UE_LOG(Myth, Error, TEXT(">>> No pawn"));
        return;
    }

    UWorld *World = PC->GetWorld();
    if (!World) {
        return;
    }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    if (!MassSubsystem) {
        UE_LOG(Myth, Error, TEXT(">>> MASS Entity Subsystem not found"));
        return;
    }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetTerritoryGrid()) {
        UE_LOG(Myth, Error, TEXT(">>> Living World not available"));
        return;
    }

    const FMythicCellCoord PlayerCell = LW->GetTerritoryGrid()->WorldToCell(PC->GetPawn()->GetActorLocation());

    TSharedPtr<FMassEntityManager> EntityManagerPtr = TSharedPtr<FMassEntityManager>(&MassSubsystem->GetMutableEntityManager(), [](FMassEntityManager *) {});

    FMassEntityQuery PressureQuery(EntityManagerPtr);
    PressureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    PressureQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadOnly);
    PressureQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== NEARBY ENTITY PRESSURE ==="));

    static const FString ChannelNames[] = {
        TEXT("Threat"), TEXT("Injustice"), TEXT("Grief"), TEXT("Shame"), TEXT("Desire"), TEXT("Wrath")
    };

    int32 DisplayCount = 0;
    const int32 MaxDisplay = 5;

    FMassEntityManager &EM_Pressure = MassSubsystem->GetMutableEntityManager();
    FMassExecutionContext TempContext(EM_Pressure);
    PressureQuery.ForEachEntityChunk(TempContext, [&](FMassExecutionContext &ChunkContext) {
        if (DisplayCount >= MaxDisplay) { return; }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        const auto PsychoView = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities && DisplayCount < MaxDisplay; ++i) {
            const FMythicCellCoord &Cell = IdentityView[i].Cell;
            const int32 Dist = FMath::Abs(Cell.X - PlayerCell.X) + FMath::Abs(Cell.Y - PlayerCell.Y);
            if (Dist > 3) { continue; }

            ++DisplayCount;
            const FMythicPsychodynamicFragment &Psycho = PsychoView[i];
            FString PressureStr;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                if (c > 0) { PressureStr += TEXT(" | "); }
                PressureStr += FString::Printf(TEXT("%s=%.2f"), *ChannelNames[c], Psycho.Pressure[c]);
            }
            UE_LOG(Myth, Warning, TEXT("  Entity at (%d,%d): %s | LastEvent=%.1f"),
                   Cell.X, Cell.Y, *PressureStr, Psycho.LastEventTime);
        }
    });

    if (DisplayCount == 0) {
        UE_LOG(Myth, Warning, TEXT("  (no hydrated entities nearby)"));
    }
    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldSignificance() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) {
        UE_LOG(Myth, Error, TEXT(">>> No pawn"));
        return;
    }

    UWorld *World = PC->GetWorld();
    if (!World) {
        return;
    }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    if (!MassSubsystem) {
        UE_LOG(Myth, Error, TEXT(">>> MASS Entity Subsystem not found"));
        return;
    }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetTerritoryGrid()) {
        UE_LOG(Myth, Error, TEXT(">>> Living World not available"));
        return;
    }

    const FMythicCellCoord PlayerCell = LW->GetTerritoryGrid()->WorldToCell(PC->GetPawn()->GetActorLocation());

    TSharedPtr<FMassEntityManager> EntityManagerPtr = TSharedPtr<FMassEntityManager>(&MassSubsystem->GetMutableEntityManager(), [](FMassEntityManager *) {});

    FMassEntityQuery SigQuery(EntityManagerPtr);
    SigQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    SigQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);

    static const FString TierNames[] = {TEXT("Ambient"), TEXT("Reactive"), TEXT("Cognitive"), TEXT("Persistent")};

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== NEARBY ENTITY SIGNIFICANCE ==="));

    int32 DisplayCount = 0;
    const int32 MaxDisplay = 10;

    int32 TierCounts[4] = {};

    FMassEntityManager &EM_Sig = MassSubsystem->GetMutableEntityManager();
    FMassExecutionContext TempContext(EM_Sig);
    SigQuery.ForEachEntityChunk(TempContext, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        const auto SigView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            const int32 TierIdx = FMath::Clamp(static_cast<int32>(SigView[i].Tier), 0, 3);
            TierCounts[TierIdx]++;

            if (DisplayCount >= MaxDisplay) { continue; }

            const FMythicCellCoord &Cell = IdentityView[i].Cell;
            const int32 Dist = FMath::Abs(Cell.X - PlayerCell.X) + FMath::Abs(Cell.Y - PlayerCell.Y);
            if (Dist > 3) { continue; }

            ++DisplayCount;
            UE_LOG(Myth, Warning, TEXT("  (%d,%d) Score=%.2f Tier=%s Events=%d Dirty=%s"),
                   Cell.X, Cell.Y,
                   SigView[i].Score,
                   *TierNames[TierIdx],
                   SigView[i].RelevantEventCount,
                   SigView[i].bDirty ? TEXT("Y") : TEXT("N"));
        }
    });

    UE_LOG(Myth, Warning, TEXT("  --- Tier Summary ---"));
    for (int32 t = 0; t < 4; ++t) {
        UE_LOG(Myth, Warning, TEXT("    %s: %d"), *TierNames[t], TierCounts[t]);
    }
    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldForcePromote() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC || !PC->GetPawn()) {
        UE_LOG(Myth, Error, TEXT(">>> No pawn"));
        return;
    }

    UWorld *World = PC->GetWorld();
    if (!World) {
        return;
    }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    if (!MassSubsystem) {
        UE_LOG(Myth, Error, TEXT(">>> MASS Entity Subsystem not found"));
        return;
    }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetTerritoryGrid()) {
        UE_LOG(Myth, Error, TEXT(">>> Living World not available"));
        return;
    }

    const FMythicCellCoord PlayerCell = LW->GetTerritoryGrid()->WorldToCell(PC->GetPawn()->GetActorLocation());

    TSharedPtr<FMassEntityManager> EntityManagerPtr = TSharedPtr<FMassEntityManager>(&MassSubsystem->GetMutableEntityManager(), [](FMassEntityManager *) {});

    FMassEntityQuery AmbientQuery(EntityManagerPtr);
    AmbientQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AmbientQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    AmbientQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::None);

    bool bPromoted = false;
    FMassEntityManager &EM_Promote = MassSubsystem->GetMutableEntityManager();
    FMassExecutionContext TempContext(EM_Promote);
    AmbientQuery.ForEachEntityChunk(TempContext, [&](FMassExecutionContext &ChunkContext) {
        if (bPromoted) { return; }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto SigView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            const FMythicCellCoord &Cell = IdentityView[i].Cell;
            const int32 Dist = FMath::Abs(Cell.X - PlayerCell.X) + FMath::Abs(Cell.Y - PlayerCell.Y);
            if (Dist > 2) { continue; }

            const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
            FMassEntityManager &EM = MassSubsystem->GetMutableEntityManager();

            TSharedPtr<FMassCommandBuffer> CmdBuffer = MakeShared<FMassCommandBuffer>();
            CmdBuffer->AddTag<FMythicHydratedTag>(Entity);
            CmdBuffer->AddFragment<FMythicPsychodynamicFragment>(Entity);
            CmdBuffer->AddFragment<FMythicPersonalityFragment>(Entity);
            CmdBuffer->AddFragment<FMythicSocialFragment>(Entity);
            EM.FlushCommands(CmdBuffer);

            SigView[i].Tier = EMythicSignificanceTier::Tier1_Reactive;
            SigView[i].Score = 1.0f;
            SigView[i].bDirty = false;

            bPromoted = true;
            UE_LOG(Myth, Warning, TEXT(">>> Force-promoted entity at (%d,%d) to Tier1_Reactive"), Cell.X, Cell.Y);
            return;
        }
    });

    if (!bPromoted) {
        UE_LOG(Myth, Warning, TEXT(">>> No Tier 0 entities found near player to promote"));
    }
}


void UMythicCheatManager::MythLivingWorldSocialGraph() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetSocialGraph()) {
        UE_LOG(Myth, Error, TEXT(">>> Social Graph not available"));
        return;
    }

    const UMythicSocialGraph *Graph = LW->GetSocialGraph();
    const int32 TotalEdges = Graph->GetTotalEdgeCount();
    const int32 EntityCount = Graph->GetEntityCount();

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== SOCIAL GRAPH ==="));
    UE_LOG(Myth, Warning, TEXT("  Entities with edges: %d"), EntityCount);
    UE_LOG(Myth, Warning, TEXT("  Total edges: %d"), TotalEdges);
    UE_LOG(Myth, Warning, TEXT("  Avg degree: %.1f"), EntityCount > 0 ? static_cast<float>(TotalEdges) / EntityCount : 0.0f);
    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldSchemes() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UMythicLivingWorldSubsystem *LW = PC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LW || !LW->GetSchemeEngine()) {
        UE_LOG(Myth, Error, TEXT(">>> Scheme Engine not available"));
        return;
    }

    TArray<FMythicScheme> Schemes = LW->GetSchemeEngine()->GetActiveSchemes();

    static const FString SchemeTypeNames[] = {
        TEXT("Assassination"), TEXT("TradeDisruption"), TEXT("TerritoryReclaim"),
        TEXT("SpyInfiltration"), TEXT("CompanionRecruitment"), TEXT("MilitaryRaid"), TEXT("DiplomaticPressure")
    };

    static const FString SchemeStateNames[] = {
        TEXT("Planning"), TEXT("InProgress"), TEXT("Succeeded"), TEXT("Failed"), TEXT("Discovered")
    };

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== ACTIVE SCHEMES (%d) ==="), Schemes.Num());

    for (const FMythicScheme &S : Schemes) {
        const FString &TypeName = SchemeTypeNames[FMath::Clamp(static_cast<int32>(S.Type), 0, SchemeTypeCount - 1)];
        const FString &StateName = SchemeStateNames[FMath::Clamp(static_cast<int32>(S.State), 0, 4)];

        UE_LOG(Myth, Warning, TEXT("  [%d] %s (%s): F%d→F%d | Progress: %.0f%% | Risk: %.2f"),
               S.SchemeId, *TypeName, *StateName,
               S.OriginFaction.Index, S.TargetFaction.Index,
               S.Progress * 100.0f, S.DetectionRisk);
    }

    if (Schemes.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("  (no active schemes)"));
    }

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldEncounters() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UWorld *World = PC->GetWorld();
    if (!World) { return; }

    UMythicEncounterDirector *Director = World->GetSubsystem<UMythicEncounterDirector>();
    if (!Director) {
        UE_LOG(Myth, Error, TEXT(">>> Encounter Director not available"));
        return;
    }

    const TArray<FMythicActiveEncounter> &Encounters = Director->GetActiveEncounters();

    static const FString StateNames[] = {
        TEXT("Pending"), TEXT("Spawning"), TEXT("Active"), TEXT("Completing"), TEXT("Completed")
    };

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== ACTIVE ENCOUNTERS (%d) ==="), Encounters.Num());

    for (const FMythicActiveEncounter &E : Encounters) {
        const FString &StateName = StateNames[FMath::Clamp(static_cast<int32>(E.State), 0, 4)];

        UE_LOG(Myth, Warning, TEXT("  [%d] %s (%s) at (%d,%d) | Faction %d | Entities: %d"),
               E.EncounterId,
               *E.TemplateTag.ToString(),
               *StateName,
               E.Cell.X, E.Cell.Y,
               E.OriginFaction.Index,
               E.EntityCount);
    }

    if (Encounters.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("  (no active encounters)"));
    }

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythLivingWorldParty() {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) { return; }

    UWorld *World = PC->GetWorld();
    if (!World) { return; }

    UMythicPartySubsystem *PartySys = World->GetSubsystem<UMythicPartySubsystem>();
    if (!PartySys) {
        UE_LOG(Myth, Error, TEXT(">>> Party Subsystem not available"));
        return;
    }

    FString PlayerKey;
    if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
        PlayerKey = PS->GetCanonicalPlayerKey();
    }

    TArray<FMythicPartyMember> Members;
    const int32 PartySize = PartySys->GetPartyMembers(PlayerKey, Members);

    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("=== PARTY (Player %s, %d members) ==="), *PlayerKey, PartySize);

    for (int32 i = 0; i < Members.Num(); ++i) {
        const FMythicPartyMember &M = Members[i];
        UE_LOG(Myth, Warning, TEXT("  [%d] %s | Loyalty: %.2f | Betrayal: %.2f | Beliefs: %d"),
               i,
               M.NPCActor.IsValid() ? TEXT("Valid") : TEXT("Invalid"),
               M.LoyaltyScore,
               M.BetrayalPressure,
               M.SharedBeliefs.Num());
    }

    if (PartySize == 0) {
        UE_LOG(Myth, Warning, TEXT("  (no companions)"));
    }

    UE_LOG(Myth, Warning, TEXT(""));
}

void UMythicCheatManager::MythDeployPlaceable(int32 SlotIndex) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOuterAPlayerController());
    if (!PC) {
        UE_LOG(Myth, Error, TEXT(">>> DeployPlaceable: no Mythic player controller"));
        return;
    }
    APawn *Pawn = PC->GetPawn();
    if (!Pawn) {
        UE_LOG(Myth, Error, TEXT(">>> DeployPlaceable: no pawn"));
        return;
    }

    const TArray<UMythicInventoryComponent *> Inventories = PC->GetAllInventoryComponents();
    if (Inventories.Num() == 0 || !Inventories[0]) {
        UE_LOG(Myth, Error, TEXT(">>> DeployPlaceable: no inventory"));
        return;
    }

    FVector AimOrigin;
    FRotator AimRot;
    Pawn->GetActorEyesViewPoint(AimOrigin, AimRot);

    PC->ServerDeployPlaceable(Inventories[0], SlotIndex, AimOrigin, AimRot.Vector());
    UE_LOG(Myth, Warning, TEXT(">>> DeployPlaceable: requested deploy of inventory slot %d (server validates; check the world for the spawned actor)"), SlotIndex);
}

void UMythicCheatManager::MythToggleCoopDown() {
    UMythicDeveloperSettings *Settings = GetMutableDefault<UMythicDeveloperSettings>();
    if (!Settings) {
        return;
    }
    Settings->bCoopDownStateEnabled = !Settings->bCoopDownStateEnabled;
    UE_LOG(Myth, Warning, TEXT(">>> Co-op down/revive is now %s (a lethal blow downs a player instead of killing)"),
           Settings->bCoopDownStateEnabled ? TEXT("ENABLED") : TEXT("disabled"));
}

void UMythicCheatManager::MythReviveSelf() {
    APlayerController *PC = GetOuterAPlayerController();
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        UE_LOG(Myth, Error, TEXT(">>> ReviveSelf: no pawn"));
        return;
    }
    UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(Pawn);
    if (!Life) {
        UE_LOG(Myth, Error, TEXT(">>> ReviveSelf: pawn has no LifeComponent"));
        return;
    }
    if (!Life->IsDowned()) {
        UE_LOG(Myth, Warning, TEXT(">>> ReviveSelf: not downed (enable ToggleCoopDown, then take lethal damage to go down first)"));
        return;
    }
    Life->ServerReviveFromDowned();
    UE_LOG(Myth, Warning, TEXT(">>> ReviveSelf: revived"));
}

void UMythicCheatManager::MythSetHealth(float Fraction) {
    APlayerController *PC = GetOuterAPlayerController();
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        UE_LOG(Myth, Error, TEXT(">>> MythSetHealth: no pawn"));
        return;
    }
    if (!Pawn->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT(">>> MythSetHealth needs authority - run it on the server or in standalone"));
        return;
    }
    UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(Pawn);
    if (!Life || !Life->IsInitialized()) {
        UE_LOG(Myth, Error, TEXT(">>> MythSetHealth: pawn has no initialised LifeComponent"));
        return;
    }
    if (!FMath::IsFinite(Fraction)) {
        UE_LOG(Myth, Error, TEXT(">>> MythSetHealth: '%f' is not a number"), Fraction);
        return;
    }
    const float Clamped = FMath::Clamp(Fraction > 1.0f ? Fraction / 100.0f : Fraction, 0.0f, 1.0f);
    Life->ServerSetHealthFraction(Clamped);
    UE_LOG(Myth, Warning, TEXT(">>> Health %.1f / %.1f (%.0f%%)"), Life->GetHealth(), Life->GetMaxHealth(), Clamped * 100.0f);
}

void UMythicCheatManager::MythAdvanceWorldTier() {
    APlayerController *PC = GetOuterAPlayerController();
    UWorld *World = PC ? PC->GetWorld() : nullptr;
    AMythicGameState *GameState = World ? World->GetGameState<AMythicGameState>() : nullptr;
    if (!GameState) {
        UE_LOG(Myth, Error, TEXT(">>> AdvanceWorldTier: no MythicGameState"));
        return;
    }

    GameState->AdvanceWorldTier();
    UE_LOG(Myth, Warning, TEXT(">>> AdvanceWorldTier: WorldTier now %d (highest reached %d)"),
           GameState->WorldTier, GameState->HighestWorldTier);
}

void UMythicCheatManager::MythObjective(const FString &Text, int32 Have, int32 Need, int32 bDone, const FString &Quest) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOuterAPlayerController());
    if (!PC) {
        UE_LOG(Myth, Error, TEXT(">>> MythObjective: no MythicPlayerController"));
        return;
    }
    PC->ClientNotifyObjective(FText::FromString(Text), Have, Need, bDone != 0, 0, FText::FromString(Quest));
    UE_LOG(Myth, Warning, TEXT(">>> MythObjective: '%s' %d/%d%s (%s)"), *Text, Have, Need, bDone ? TEXT(" done") : TEXT(""), *Quest);
}

void UMythicCheatManager::MythOpenMenu(const FString &PageId) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOuterAPlayerController());
    if (!PC) {
        UE_LOG(Myth, Error, TEXT(">>> MythOpenMenu: no MythicPlayerController"));
        return;
    }
    UMythicHUDLayout *Layout = nullptr;
    for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
        if (IsValid(*It) && !It->HasAnyFlags(RF_ClassDefaultObject) && It->GetOwningPlayer() == PC) {
            Layout = *It;
            break;
        }
    }
    if (!Layout) {
        UE_LOG(Myth, Error, TEXT(">>> MythOpenMenu: no HUD layout for this player"));
        return;
    }
    Layout->OpenMenuOnPage(PageId.IsEmpty() ? NAME_None : FName(*PageId));
    UE_LOG(Myth, Warning, TEXT(">>> MythOpenMenu: '%s'"), PageId.IsEmpty() ? TEXT("(default)") : *PageId);
}


void UMythicCheatManager::MythDumpActions() {
    const ULocalPlayer *LP = GetOuterAPlayerController() ? GetOuterAPlayerController()->GetLocalPlayer() : nullptr;
    if (!LP) {
        UE_LOG(Myth, Warning, TEXT("DumpActions: no local player"));
        return;
    }
    const UCommonUIActionRouterBase *Router = ULocalPlayer::GetSubsystem<UCommonUIActionRouterBase>(LP);
    if (!Router) {
        UE_LOG(Myth, Warning, TEXT("DumpActions: no action router"));
        return;
    }

    const UCommonInputSubsystem &Input = Router->GetInputSubsystem();
    UE_LOG(Myth, Warning, TEXT("DumpActions: enhancedInputSupport=%d inputType=%d leafmost=%s"),
           CommonUI::IsEnhancedInputSupportEnabled() ? 1 : 0,
           static_cast<int32>(Input.GetCurrentInputType()),
           *GetNameSafe(Router->GetLeafmostActivatableWidget()));

    // Which layer holds what. A screen that is visible but absent here was never pushed - it was parented
    // into some panel instead, and the action router cannot see it or anything it hosts.
    if (UPrimaryGameLayout *Layout = UPrimaryGameLayout::GetPrimaryGameLayout(GetOuterAPlayerController())) {
        static const TCHAR *LayerNames[] = {TEXT("UI.Layer.Game"), TEXT("UI.Layer.GameMenu"),
                                            TEXT("UI.Layer.Menu"), TEXT("UI.Layer.Modal")};
        for (const TCHAR *Name : LayerNames) {
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(Name), false);
            UCommonActivatableWidgetContainerBase *Layer =
                Tag.IsValid() ? Layout->GetLayerWidget(Tag) : nullptr;
            const TSharedPtr<SWidget> LayerSlate = Layer ? Layer->GetCachedWidget() : nullptr;
            UE_LOG(Myth, Warning, TEXT("   layer %-18s %s active=%-24s containerPaintLayer=%d vis=%s"), Name,
                   Layer ? TEXT("present") : TEXT("MISSING"),
                   Layer ? *GetNameSafe(Layer->GetActiveWidget()) : TEXT("-"),
                   (LayerSlate.IsValid() && LayerSlate->GetVisibility().IsVisible())
                       ? LayerSlate->GetPersistentState().LayerId : INDEX_NONE,
                   LayerSlate.IsValid() ? *LayerSlate->GetVisibility().ToString() : TEXT("-"));
        }
    }
    else {
        UE_LOG(Myth, Warning, TEXT("   no PrimaryGameLayout"));
    }

    for (TObjectIterator<UMythicActivatableWidget> It; It; ++It) {
        const UMythicActivatableWidget *W = *It;
        if (!W || W->HasAnyFlags(RF_ClassDefaultObject) || W->GetWorld() != GetWorld()) {
            continue;
        }
        const FString N = W->GetName();
        if (!N.Contains(TEXT("EscapeMenu")) && !N.Contains(TEXT("PlayerHUD")) && !N.Contains(TEXT("Settings"))) {
            continue;
        }
        // Reproduces FActivatableTreeNode::GetLastPaintLayer: a cached Slate widget that is not visible
        // scores INDEX_NONE, so its root can never out-rank the layer below it no matter how it is stacked.
        const TSharedPtr<SWidget> Cached = W->GetCachedWidget();
        const bool bVisible = Cached.IsValid() && Cached->GetVisibility().IsVisible();
        const int32 PaintLayer = bVisible ? Cached->GetPersistentState().LayerId : INDEX_NONE;
        UE_LOG(Myth, Warning, TEXT("   widget %-30s activated=%d inActiveRoot=%d cached=%d vis=%s paintLayer=%d"),
               *W->GetName(), W->IsActivated() ? 1 : 0, Router->IsWidgetInActiveRoot(W) ? 1 : 0,
               Cached.IsValid() ? 1 : 0,
               Cached.IsValid() ? *UEnum::GetValueAsString(W->GetVisibility()) : TEXT("-"), PaintLayer);
    }

    const TArray<FUIActionBindingHandle> Handles = Router->GatherActiveBindings();
    UE_LOG(Myth, Warning, TEXT("DumpActions: %d active bindings"), Handles.Num());

    UEnhancedInputLocalPlayerSubsystem *EI = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    for (const FUIActionBindingHandle &Handle : Handles) {
        const TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(Handle);
        if (!Binding.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("   <stale handle>"));
            continue;
        }
        const UInputAction *Action = Binding->InputAction.Get();
        const int32 Keys = (Action && EI) ? EI->QueryKeysMappedToAction(Action).Num() : -1;
        UE_LOG(Myth, Warning, TEXT("   '%s' inActionBar=%d action=%s keys=%d validForInput=%d"),
               *Binding->ActionName.ToString(), Binding->bDisplayInActionBar ? 1 : 0,
               *GetNameSafe(Action), Keys,
               CommonUI::ActionValidForInputType(LP, Input.GetCurrentInputType(), Action) ? 1 : 0);
    }
}

namespace {
TArray<UTalentDefinition *> Cheat_AllTalents() {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UTalentDefinition::StaticClass()->GetClassPathName(), Assets);

    TArray<UTalentDefinition *> Out;
    for (const FAssetData &Asset : Assets) {
        if (UTalentDefinition *Talent = Cast<UTalentDefinition>(Asset.GetAsset())) {
            Out.Add(Talent);
        }
    }
    Out.Sort([](const UTalentDefinition &A, const UTalentDefinition &B) { return A.GetName() < B.GetName(); });
    return Out;
}
}

void UMythicCheatManager::MythListTalents() {
    const TArray<UTalentDefinition *> Talents = Cheat_AllTalents();
    UE_LOG(Myth, Log, TEXT(">>> %d talents"), Talents.Num());
    for (const UTalentDefinition *Talent : Talents) {
        UE_LOG(Myth, Log, TEXT("  %-28s %-16s %s"), *Talent->GetName(), *Talent->Name.ToString(),
               *Talent->AbilityDef.RichText.ToString());
    }
}

void UMythicCheatManager::MythGiveTalent(const FString &TalentName) {
    UMythicAbilitySystemComponent *ASC = Cheat_PlayerASC(GetOuterAPlayerController());
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT(">>> No ASC"));
        return;
    }

    const TArray<UTalentDefinition *> Talents = Cheat_AllTalents();
    UTalentDefinition *Match = nullptr;
    for (UTalentDefinition *Talent : Talents) {
        if (Talent->GetName().Contains(TalentName) || Talent->Name.ToString().Contains(TalentName)) {
            Match = Talent;
            break;
        }
    }
    if (!Match) {
        UE_LOG(Myth, Error, TEXT(">>> No talent matching '%s'. MythListTalents for the list."), *TalentName);
        return;
    }
    if (!Match->AbilityDef.Ability) {
        UE_LOG(Myth, Error, TEXT(">>> '%s' has no ability to grant"), *Match->GetName());
        return;
    }

    FGameplayAbilitySpec Spec(Match->AbilityDef.Ability, 1, INDEX_NONE, Match);
    ASC->GiveAbility(Spec);
    UE_LOG(Myth, Log, TEXT(">>> Granted %s: %s"), *Match->GetName(), *Match->AbilityDef.RichText.ToString());
}

namespace {
AMythicPlayerState *Cheat_PlayerState(APlayerController *PC) {
    return PC ? Cast<AMythicPlayerState>(PC->PlayerState) : nullptr;
}

// Rune verbs, deeds and the ledger are all authority-gated, so a client console silently does nothing without this.
AMythicPlayerState *Cheat_AuthorityPlayerState(APlayerController *PC, const TCHAR *Cheat) {
    AMythicPlayerState *PS = Cheat_PlayerState(PC);
    if (!PS) {
        UE_LOG(Myth, Error, TEXT(">>> No player state"));
        return nullptr;
    }
    if (!PS->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT(">>> %s needs authority - run it on the server or in standalone"), Cheat);
        return nullptr;
    }
    return PS;
}

TArray<UMythicRuneDefinition *> Cheat_AllRunes() {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UMythicRuneDefinition::StaticClass()->GetClassPathName(), Assets);

    TArray<UMythicRuneDefinition *> Out;
    for (const FAssetData &Asset : Assets) {
        if (UMythicRuneDefinition *Rune = Cast<UMythicRuneDefinition>(Asset.GetAsset())) {
            Out.Add(Rune);
        }
    }
    Out.Sort([](const UMythicRuneDefinition &A, const UMythicRuneDefinition &B) { return A.GetName() < B.GetName(); });
    return Out;
}

UMythicRuneDefinition *Cheat_FindRune(const FString &Name) {
    for (UMythicRuneDefinition *Rune : Cheat_AllRunes()) {
        if (Rune->GetName().Contains(Name) || Rune->Name.ToString().Contains(Name)) {
            return Rune;
        }
    }
    UE_LOG(Myth, Error, TEXT(">>> No rune matching '%s'. MythListRunes for the list."), *Name);
    return nullptr;
}

int32 Cheat_WornSlot(const UMythicRuneComponent *Runes, const UMythicRuneDefinition *Rune) {
    const FSoftObjectPath Path(Rune);
    const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &Worn = Runes->GetEquippedRunes();
    for (int32 Slot = 0; Slot < Worn.Num(); Slot++) {
        if (Worn[Slot].ToSoftObjectPath() == Path) {
            return Slot;
        }
    }
    return INDEX_NONE;
}

// The same three axes HandleTriggerEvent gathers, so "open now" here is what the clause will see.
FGameplayTagContainer Cheat_WorldTags(const APlayerController *PC) {
    FGameplayTagContainer Tags;
    const UGameInstance *GI = PC ? PC->GetGameInstance() : nullptr;
    if (const UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr) {
        Tags.AddTag(Env->GetWeather());
        Tags.AddTag(Env->GetDayTimeTag());
        Tags.AddTag(Env->GetSeasonTag());
    }
    return Tags;
}

// A v2 rune has no authored gate to print; what it has is a HUD cell, a timer and its rolls. All three come off the
// live rune component, so this reads what the player's screen is reading.
FString Cheat_RuneStateSummary(const UMythicRuneComponent *Runes, const UMythicRuneDefinition *Rune, int32 Slot) {
    const FSoftObjectPath Path(Rune);
    const FMythicRuneRollSet *Set = Runes->GetRuneRolls().FindByPredicate([&Path](const FMythicRuneRollSet &Candidate) {
        return Candidate.Rune.ToSoftObjectPath() == Path;
    });
    const FString Rolls = FString::Printf(TEXT("%d of %d rolled"), Set ? Set->Values.Num() : 0, Rune->Parameters.Num());
    if (Slot == INDEX_NONE) {
        return FString::Printf(TEXT("%-9s %-9s %s"), TEXT("-"), TEXT("-"), *Rolls);
    }

    const float Remaining = Runes->GetRuneHudRemainingSeconds(Slot);
    return FString::Printf(TEXT("%-9s %-9s %s"),
                           *StaticEnum<EMythicRuneHudState>()->GetNameStringByValue(static_cast<int64>(Runes->GetRuneHudState(Slot))),
                           Remaining > 0.0f ? *FString::Printf(TEXT("%.1fs left"), Remaining) : TEXT("-"), *Rolls);
}

UMythicAchievementSet *Cheat_AchievementSet() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    UMythicAchievementSet *Set = Settings && !Settings->DefaultAchievementSet.IsNull()
                                     ? Settings->DefaultAchievementSet.LoadSynchronous()
                                     : nullptr;
    if (!Set) {
        UE_LOG(Myth, Error, TEXT(">>> No DefaultAchievementSet in Project Settings > Mythic"));
    }
    return Set;
}

UMythicAchievementDefinition *Cheat_FindAchievement(const FString &Name) {
    UMythicAchievementSet *Set = Cheat_AchievementSet();
    if (!Set) {
        return nullptr;
    }
    for (UMythicAchievementDefinition *Def : Set->Achievements) {
        if (Def && (Def->GetName().Contains(Name) || Def->DisplayName.ToString().Contains(Name)
                    || Def->AchievementTag.ToString().Contains(Name))) {
            return Def;
        }
    }
    UE_LOG(Myth, Error, TEXT(">>> No achievement matching '%s'. Known:"), *Name);
    for (const UMythicAchievementDefinition *Def : Set->Achievements) {
        if (Def) {
            UE_LOG(Myth, Warning, TEXT("      %-24s %s"), *Def->GetName(), *Def->AchievementTag.ToString());
        }
    }
    return nullptr;
}
}

void UMythicCheatManager::MythListRunes() {
    APlayerController *PC = GetOuterAPlayerController();
    const AMythicPlayerState *PS = Cheat_PlayerState(PC);
    const UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        UE_LOG(Myth, Error, TEXT(">>> No rune component"));
        return;
    }

    const FGameplayTagContainer WorldTags = Cheat_WorldTags(PC);
    const TArray<UMythicRuneDefinition *> All = Cheat_AllRunes();
    UE_LOG(Myth, Warning, TEXT(">>> %d runes, %d of %d sockets open, world: %s"), All.Num(), Runes->GetUnlockedSlots(),
           Runes->MaxSlots, *WorldTags.ToStringSimple());
    for (const UMythicRuneDefinition *Rune : All) {
        const int32 Slot = Cheat_WornSlot(Runes, Rune);
        UE_LOG(Myth, Warning, TEXT("    %-24s %-22s %-8s %-10s %s"), *Rune->GetName(),
               *Rune->RequiredTag.ToString(), Runes->IsRuneUnlocked(Rune) ? TEXT("earned") : TEXT("locked"),
               Slot == INDEX_NONE ? TEXT("-") : *FString::Printf(TEXT("socket %d"), Slot),
               *Cheat_RuneStateSummary(Runes, Rune, Slot));
    }
}

void UMythicCheatManager::MythGiveRune(const FString &Name, int32 Slot) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythGiveRune"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }
    UMythicRuneDefinition *Rune = Cheat_FindRune(Name);
    if (!Rune) {
        return;
    }

    if (Slot < 0) {
        for (int32 Index = 0; Index < Runes->GetUnlockedSlots(); Index++) {
            if (!Runes->GetRuneInSlot(Index)) {
                Slot = Index;
                break;
            }
        }
        if (Slot < 0) {
            UE_LOG(Myth, Error, TEXT(">>> Every open socket is worn. Name a socket, MythRemoveRune one, or MythRuneSlots more"));
            return;
        }
    }

    int32 OtherSlot = INDEX_NONE;
    const EMythicRuneRefusal Reason = Runes->CanEquipRune(Slot, Rune, OtherSlot);
    Runes->ServerEquipRune(Slot, Rune);
    if (Reason == EMythicRuneRefusal::None) {
        UE_LOG(Myth, Log, TEXT(">>> %s worn in socket %d"), *Rune->GetName(), Slot);
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> %s refused for socket %d: %s"), *Rune->GetName(), Slot,
               *UMythicRuneComponent::DescribeRefusal(Reason, OtherSlot).ToString());
    }
}

void UMythicCheatManager::MythRemoveRune(int32 Slot) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythRemoveRune"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }
    const UMythicRuneDefinition *Worn = Runes->GetRuneInSlot(Slot);
    if (!Worn) {
        UE_LOG(Myth, Warning, TEXT(">>> Socket %d holds nothing"), Slot);
        return;
    }
    Runes->ServerUnequipRune(Slot);
    UE_LOG(Myth, Log, TEXT(">>> %s taken out of socket %d"), *Worn->GetName(), Slot);
}

void UMythicCheatManager::MythMoveRune(int32 From, int32 To) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythMoveRune"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }
    const UMythicRuneDefinition *Worn = Runes->GetRuneInSlot(From);
    Runes->ServerMoveRune(From, To);
    if (Worn && From != To && Runes->GetRuneInSlot(To) == Worn && !Runes->GetRuneInSlot(From)) {
        UE_LOG(Myth, Log, TEXT(">>> %s moved from socket %d to %d"), *Worn->GetName(), From, To);
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> Move %d -> %d refused; see the Runes: line above"), From, To);
    }
}

void UMythicCheatManager::MythRuneSlots(int32 Count) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythRuneSlots"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }
    const int32 Target = FMath::Clamp(Count, 1, Runes->MaxSlots);
    while (Runes->GetUnlockedSlots() < Target) {
        const int32 Before = Runes->GetUnlockedSlots();
        Runes->GrantSlot();
        if (Runes->GetUnlockedSlots() == Before) {
            break;
        }
    }
    while (Runes->GetUnlockedSlots() > Target) {
        const int32 Before = Runes->GetUnlockedSlots();
        Runes->RevokeSlot();
        if (Runes->GetUnlockedSlots() == Before) {
            break;
        }
        UE_LOG(Myth, Log, TEXT(">>> socket %d closed"), Before - 1);
    }
    UE_LOG(Myth, Log, TEXT(">>> %d of %d sockets open"), Runes->GetUnlockedSlots(), Runes->MaxSlots);
}

void UMythicCheatManager::MythRuneHud() {
    const AMythicPlayerState *PS = Cheat_PlayerState(GetOuterAPlayerController());
    const UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        UE_LOG(Myth, Error, TEXT(">>> No rune component"));
        return;
    }
    const UEnum *States = StaticEnum<EMythicRuneHudState>();
    const TArray<FMythicRuneHudStateItem> &Rows = Runes->GetRuneHudStates();
    UE_LOG(Myth, Warning, TEXT(">>> %d of %d sockets open, %d HUD rows, server clock %.1fs"), Runes->GetUnlockedSlots(),
           Runes->MaxSlots, Rows.Num(), Runes->GetServerWorldTimeSeconds());
    for (int32 Slot = 0; Slot < Runes->GetUnlockedSlots(); Slot++) {
        const UMythicRuneDefinition *Rune = Runes->GetRuneInSlot(Slot);
        const FMythicRuneHudStateItem *Row = Rows.FindByPredicate([Slot](const FMythicRuneHudStateItem &Item) {
            return Item.SlotIndex == Slot;
        });
        UE_LOG(Myth, Warning, TEXT("    socket %d  %-24s %-9s %6.1fs left  x%d"), Slot, Rune ? *Rune->GetName() : TEXT("-"),
               *States->GetNameStringByValue(static_cast<int64>(Runes->GetRuneHudState(Slot))),
               Runes->GetRuneHudRemainingSeconds(Slot), Row ? Row->Stacks : 0);
    }
}

void UMythicCheatManager::MythRuneRolls() {
    const AMythicPlayerState *PS = Cheat_PlayerState(GetOuterAPlayerController());
    const UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        UE_LOG(Myth, Error, TEXT(">>> No rune component"));
        return;
    }

    const TArray<FMythicRuneRollSet> &Sets = Runes->GetRuneRolls();
    UE_LOG(Myth, Warning, TEXT(">>> %d roll sets, %d of %d sockets open"), Sets.Num(), Runes->GetUnlockedSlots(),
           Runes->MaxSlots);
    for (const FMythicRuneRollSet &Set : Sets) {
        const UMythicRuneDefinition *Rune = Set.Rune.LoadSynchronous();
        if (!Rune) {
            UE_LOG(Myth, Error, TEXT("    %s: definition missing, %d values orphaned"), *Set.Rune.ToString(), Set.Values.Num());
            continue;
        }
        const int32 Slot = Runes->FindSlotOfRune(Rune);
        UE_LOG(Myth, Warning, TEXT(">>> %-24s %-10s %d of %d parameters rolled"), *Rune->GetName(),
               Slot == INDEX_NONE ? TEXT("not worn") : *FString::Printf(TEXT("socket %d"), Slot), Set.Values.Num(),
               Rune->Parameters.Num());
        for (const FMythicRuneRollValue &Roll : Set.Values) {
            const FRollDefinition *Range = Rune->Parameters.Find(Roll.Parameter);
            const FString RangeText = Range
                                          ? FString::Printf(TEXT("[%g-%g]%s"), Range->Min, Range->Max,
                                                            Range->bWholeNumber ? TEXT(" whole") : TEXT(""))
                                          : FString(TEXT("not in Parameters"));
            UE_LOG(Myth, Warning, TEXT("    %-36s %-12g %s"), *Roll.Parameter.ToString(), Roll.Value, *RangeText);
        }
        for (const TPair<FGameplayTag, FRollDefinition> &Param : Rune->Parameters) {
            const bool bRolled = Set.Values.ContainsByPredicate(
                [&Param](const FMythicRuneRollValue &Roll) { return Roll.Parameter == Param.Key; });
            if (!bRolled) {
                UE_LOG(Myth, Warning, TEXT("    %-36s unrolled, reads midpoint %g of [%g-%g]"), *Param.Key.ToString(),
                       Rune->GetParameterMidpoint(Param.Key, 0.0f), Param.Value.Min, Param.Value.Max);
            }
        }
    }

    for (int32 Slot = 0; Slot < Runes->GetUnlockedSlots(); Slot++) {
        const UMythicRuneDefinition *Rune = Runes->GetRuneInSlot(Slot);
        if (!Rune || Rune->Parameters.IsEmpty()) {
            continue;
        }
        const FSoftObjectPath Path(Rune);
        const bool bHasSet = Sets.ContainsByPredicate(
            [&Path](const FMythicRuneRollSet &Set) { return Set.Rune.ToSoftObjectPath() == Path; });
        if (!bHasSet) {
            UE_LOG(Myth, Error, TEXT(">>> socket %d %s has %d parameters and no roll set; ReadRolled falls back to midpoints"),
                   Slot, *Rune->GetName(), Rune->Parameters.Num());
        }
    }
    if (Sets.IsEmpty()) {
        UE_LOG(Myth, Warning, TEXT(">>> Nothing rolled yet; MythGiveRune <Name> rolls at first socket"));
    }
}

void UMythicCheatManager::MythGiveDeed(const FString &AchievementName) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythGiveDeed"));
    if (!PS) {
        return;
    }
    const UMythicAchievementDefinition *Def = Cheat_FindAchievement(AchievementName);
    if (!Def) {
        return;
    }
    Cheat_GrantDeed(PS, Def, true);
}

bool UMythicCheatManager::Cheat_GrantDeed(AMythicPlayerState *PS, const UMythicAchievementDefinition *Def, bool bVerbose) {
    if (!PS || !Def) {
        return false;
    }
    UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent();
    UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState();
    const UMythicAchievementComponent *Achievements = PS->GetAchievementComponent();
    if (!Ledger || !Narrative || !Achievements) {
        UE_LOG(Myth, Error, TEXT(">>> Player state is missing a progression ledger"));
        return false;
    }
    if (Achievements->IsAchievementUnlocked(Def->AchievementTag)) {
        if (bVerbose) {
            UE_LOG(Myth, Warning, TEXT(">>> %s is already earned"), *Def->AchievementTag.ToString());
        }
        return true;
    }

    // Counters and story tags are pushed through their real writers so the whole chain fires: counter ->
    // achievement -> story tag -> unlock rule -> socket. A loose tag would satisfy the rune gate and open nothing.
    for (const FMythicStatRequirement &Req : Def->Condition.StatRequirements) {
        if (!Req.StatTag.IsValid()) {
            continue;
        }
        const int64 Current = Req.bHierarchical ? Ledger->GetCounterRollup(Req.StatTag) : Ledger->GetCounter(Req.StatTag);
        const int64 Delta = FMath::Max<int64>(0, Req.MinValue - Current);
        if (Delta > 0) {
            Ledger->RecordStat(Req.StatTag, Delta);
        }
        if (bVerbose) {
            UE_LOG(Myth, Log, TEXT(">>> %s %lld -> %lld (needs %lld)"), *Req.StatTag.ToString(), Current, Current + Delta, Req.MinValue);
        }
    }
    for (const FGameplayTag &Tag : Def->Condition.TagCondition.RequireAll) {
        if (!Narrative->HasStoryTag(Tag)) {
            Narrative->ServerSetStoryTag(Tag);
            if (bVerbose) {
                UE_LOG(Myth, Log, TEXT(">>> story tag %s set"), *Tag.ToString());
            }
        }
    }
    const FGameplayTagContainer &AnyOf = Def->Condition.TagCondition.RequireAny;
    if (!AnyOf.IsEmpty() && !Narrative->HasAny(AnyOf) && !Achievements->GetUnlockedAchievements().HasAny(AnyOf)) {
        Narrative->ServerSetStoryTag(AnyOf.First());
        if (bVerbose) {
            UE_LOG(Myth, Log, TEXT(">>> story tag %s set"), *AnyOf.First().ToString());
        }
    }

    const bool bEarned = Achievements->IsAchievementUnlocked(Def->AchievementTag);
    if (bEarned) {
        if (bVerbose) {
            UE_LOG(Myth, Log, TEXT(">>> %s earned"), *Def->AchievementTag.ToString());
        }
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> %s still locked after its requirements were met; check its BlockAny clause"),
               *Def->AchievementTag.ToString());
    }
    return bEarned;
}

void UMythicCheatManager::MythUnlockAllRunes() {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythUnlockAllRunes"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }

    const TArray<UMythicRuneDefinition *> All = Cheat_AllRunes();
    FGameplayTagContainer Granted;
    int32 Unlocked = 0;
    int32 Failed = 0;
    for (const UMythicRuneDefinition *Rune : All) {
        const FGameplayTag Deed = Rune->RequiredTag;
        if (Runes->IsRuneUnlocked(Rune)) {
            ++Unlocked;
            continue;
        }
        if (!Granted.HasTagExact(Deed)) {
            // Through the real chain so the deed also opens whatever sockets and titles it gates.
            if (const UMythicAchievementDefinition *Def = Cheat_FindAchievement(Deed.ToString())) {
                Cheat_GrantDeed(PS, Def, false);
            }
            else if (UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState()) {
                UE_LOG(Myth, Warning, TEXT(">>> %s has no achievement definition; setting it as a story tag instead"),
                       *Deed.ToString());
                Narrative->ServerSetStoryTag(Deed);
            }
            Granted.AddTag(Deed);
        }
        if (Runes->IsRuneUnlocked(Rune)) {
            ++Unlocked;
        }
        else {
            ++Failed;
            UE_LOG(Myth, Error, TEXT(">>> %s is still locked behind %s"), *Rune->GetName(), *Deed.ToString());
        }
    }
    UE_LOG(Myth, Warning, TEXT(">>> %d of %d runes unlocked from %d deeds%s"), Unlocked, All.Num(), Granted.Num(),
           Failed > 0 ? TEXT(" (see the errors above)") : TEXT(""));
}

void UMythicCheatManager::MythUnlockAllRuneSlots() {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythUnlockAllRuneSlots"));
    UMythicRuneComponent *Runes = PS ? PS->GetRuneComponent() : nullptr;
    if (!Runes) {
        return;
    }
    while (Runes->GetUnlockedSlots() < Runes->MaxSlots) {
        const int32 Before = Runes->GetUnlockedSlots();
        Runes->GrantSlot();
        if (Runes->GetUnlockedSlots() == Before) {
            UE_LOG(Myth, Error, TEXT(">>> socket %d refused to open; stopping at %d"), Before, Before);
            break;
        }
    }
    UE_LOG(Myth, Warning, TEXT(">>> %d of %d sockets open"), Runes->GetUnlockedSlots(), Runes->MaxSlots);
}

void UMythicCheatManager::MythClearDeed(const FString &AchievementName) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythClearDeed"));
    if (!PS) {
        return;
    }
    const UMythicAchievementDefinition *Def = Cheat_FindAchievement(AchievementName);
    if (!Def) {
        return;
    }
    UMythicAchievementComponent *Achievements = PS->GetAchievementComponent();
    UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState();
    UMythicRuneComponent *Runes = PS->GetRuneComponent();
    if (!Achievements || !Narrative || !Runes) {
        UE_LOG(Myth, Error, TEXT(">>> Player state is missing a progression ledger"));
        return;
    }

    FGameplayTagContainer Remaining = Achievements->GetUnlockedAchievements();
    Remaining.RemoveTag(Def->AchievementTag);
    Achievements->RestoreUnlockedAchievements(Remaining);
    Narrative->ServerClearStoryTag(Def->AchievementTag);

    // Re-restoring the worn set drops any rune whose deed just went, the same way a reload would.
    const TArray<TSoftObjectPtr<UMythicRuneDefinition>> Worn = Runes->GetEquippedRunes();
    Runes->RestoreRunes(Worn, Runes->GetUnlockedSlots());

    UE_LOG(Myth, Log, TEXT(">>> %s cleared from achievements and story tags"), *Def->AchievementTag.ToString());
    if (const UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent()) {
        for (const FGameplayTag &Rule : Unlocks->GetAppliedUnlockRules()) {
            UE_LOG(Myth, Warning, TEXT(">>> %s stays applied; unlock rules latch once and never re-check"), *Rule.ToString());
        }
    }
}

void UMythicCheatManager::MythRecordStat(const FString &StatTag, int32 Delta) {
    AMythicPlayerState *PS = Cheat_AuthorityPlayerState(GetOuterAPlayerController(), TEXT("MythRecordStat"));
    UMythicStatLedgerComponent *Ledger = PS ? PS->GetStatLedgerComponent() : nullptr;
    if (!Ledger) {
        return;
    }
    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*StatTag), false);
    if (!Tag.IsValid()) {
        UE_LOG(Myth, Error, TEXT(">>> '%s' is not a registered tag"), *StatTag);
        return;
    }
    Ledger->RecordStat(Tag, Delta);
    UE_LOG(Myth, Log, TEXT(">>> %s is now %lld"), *Tag.ToString(), Ledger->GetCounter(Tag));
}

void UMythicCheatManager::MythSetSeason(const FString &Season) {
    APlayerController *PC = GetOuterAPlayerController();
    if (!PC) {
        return;
    }
    UMythicEnvironmentSubsystem *EnvSys = PC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>();
    AMythicEnvironmentController *Controller = EnvSys ? EnvSys->GetEnvironmentController() : nullptr;
    if (!Controller) {
        UE_LOG(Myth, Error, TEXT(">>> No Environment Controller"));
        return;
    }
    if (!Controller->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT(">>> MythSetSeason needs authority - run it on the server or in standalone"));
        return;
    }

    // First month of each season as MonthAsSeason reads them; Winter wraps to December.
    int32 Month = 0;
    if (Season.Equals(TEXT("Spring"), ESearchCase::IgnoreCase)) {
        Month = 3;
    }
    else if (Season.Equals(TEXT("Summer"), ESearchCase::IgnoreCase)) {
        Month = 6;
    }
    else if (Season.Equals(TEXT("Autumn"), ESearchCase::IgnoreCase)) {
        Month = 9;
    }
    else if (Season.Equals(TEXT("Winter"), ESearchCase::IgnoreCase)) {
        Month = 12;
    }
    else {
        UE_LOG(Myth, Error, TEXT(">>> Unknown season '%s'. Spring, Summer, Autumn or Winter"), *Season);
        return;
    }

    const FDateTime Current = Controller->GetDateTime();
    const FDateTime NewTime(Current.GetYear(), Month, 1, Current.GetHour(), Current.GetMinute());
    Controller->SetTime(NewTime);
    UE_LOG(Myth, Warning, TEXT(">>> Season set to %s (%s), now %s"), *Season, *NewTime.ToString(),
           *EnvSys->GetSeasonTag().ToString());
}
