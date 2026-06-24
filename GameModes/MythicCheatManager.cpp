#include "MythicCheatManager.h"
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
#include "Mythic/Player/MythicPlayerController.h" // DeployPlaceable cheat -> ServerDeployPlaceable
#include "Mythic/GAS/AttributeSets/Shared/MythicLifeComponent.h" // ReviveSelf cheat
#include "Mythic/Settings/MythicDeveloperSettings.h"             // ToggleCoopDown cheat
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyDefinition.h"
#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#include "Mythic/Mythic.h"
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

// ============================================================================
// HELP
// ============================================================================

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
    UE_LOG(Myth, Warning, TEXT("  MythGiveItem <Name> [Count]        - Give item by name (partial match)"));
    UE_LOG(Myth, Warning, TEXT("  MythClearInventory                 - Clear all inventory items"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- ATTRIBUTES ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListAttributes                 - List all attributes and values"));
    UE_LOG(Myth, Warning, TEXT("  MythSetAttribute <Name> <Value>    - Set attribute value"));
    UE_LOG(Myth, Warning, TEXT(""));
    UE_LOG(Myth, Warning, TEXT("--- PROFICIENCIES ---"));
    UE_LOG(Myth, Warning, TEXT("  MythListProficiencies              - List proficiencies and progress"));
    UE_LOG(Myth, Warning, TEXT("  MythGiveProficiency <Name> <Amt>   - Give proficiency progress"));
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

// ============================================================================
// SAVE SYSTEM
// ============================================================================

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

// ============================================================================
// WEATHER
// ============================================================================

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

// ============================================================================
// TIME OF DAY
// ============================================================================

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

// ============================================================================
// ITEMS
// ============================================================================

void UMythicCheatManager::MythListItems() {
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    TArray<FPrimaryAssetId> ItemAssetIds;
    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ItemDefinitionType, ItemAssetIds);

    UE_LOG(Myth, Warning, TEXT(">>> Items (%d):"), ItemAssetIds.Num());
    for (const FPrimaryAssetId &AssetId : ItemAssetIds) {
        UE_LOG(Myth, Warning, TEXT("    - %s"), *AssetId.PrimaryAssetName.ToString());
    }
}

void UMythicCheatManager::MythGiveItem(const FString &ItemName, int32 Count) {
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
    for (const FPrimaryAssetId &AssetId : ItemAssetIds) {
        if (AssetId.PrimaryAssetName.ToString().Contains(ItemName, ESearchCase::IgnoreCase)) {
            // TryLoad is acceptable for debug cheat commands
            FSoftObjectPath ItemPath = AssetManager.GetPrimaryAssetPath(AssetId);
            MatchedItem = Cast<UItemDefinition>(ItemPath.TryLoad());
            if (MatchedItem) {
                break;
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
        1
        );

    if (Dropped) {
        UE_LOG(Myth, Warning, TEXT(">>> Gave %d x %s (dropped)"), Count, *MatchedItem->Name.ToString());
    }
    else {
        UE_LOG(Myth, Warning, TEXT(">>> Gave %d x %s"), Count, *MatchedItem->Name.ToString());
    }
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

// ============================================================================
// ATTRIBUTES
// ============================================================================

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

        // Iterate through all properties to find FGameplayAttributeData
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

    // Use GetAttributeSets to iterate through all attribute sets
    const TArray<UMythicAttributeSet *> &AttributeSets = ASC->GetAttributeSets();

    for (const UMythicAttributeSet *AttrSet : AttributeSets) {
        if (!AttrSet) {
            continue;
        }

        for (TFieldIterator<FProperty> PropIt(AttrSet->GetClass()); PropIt; ++PropIt) {
            FProperty *Property = *PropIt;
            if (Property->GetName().Contains(AttributeName, ESearchCase::IgnoreCase)) {
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

    UE_LOG(Myth, Error, TEXT(">>> Attribute '%s' not found. Use ListAttributes."), *AttributeName);
}

// ============================================================================
// PROFICIENCIES
// ============================================================================

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
        if (ASC && Prof.ProgressAttribute.IsValid()) {
            bool bFound = false;
            CurrentProgress = ASC->GetGameplayAttributeValue(Prof.ProgressAttribute, bFound);
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

    // Find matching proficiency
    for (FProficiency &Prof : ProfComp->Proficiencies) {
        FString Name = Prof.Definition ? Prof.Definition->GetName() : TEXT("");
        if (Name.Contains(ProficiencyName, ESearchCase::IgnoreCase)) {
            if (!Prof.ProgressAttribute.IsValid()) {
                UE_LOG(Myth, Error, TEXT(">>> Proficiency '%s' has no ProgressAttribute"), *Name);
                return;
            }

            bool bFound = false;
            float Current = ASC->GetGameplayAttributeValue(Prof.ProgressAttribute, bFound);
            if (bFound) {
                float NewVal = Current + Amount;
                ASC->SetNumericAttributeBase(Prof.ProgressAttribute, NewVal);
                UE_LOG(Myth, Warning, TEXT(">>> %s: %.0f -> %.0f (+%.0f)"), *Name, Current, NewVal, Amount);
                return;
            }
        }
    }

    UE_LOG(Myth, Error, TEXT(">>> Proficiency '%s' not found. Use ListProficiencies."), *ProficiencyName);
}

// ============================================================================
// LIVING WORLD
// ============================================================================

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

    // Causal Fabric
    if (const UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
        UE_LOG(Myth, Warning, TEXT("  Causal Fabric: initialized (capacity %d)"), Fabric->GetCapacity());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  Causal Fabric: NOT initialized"));
    }

    // Faction DB
    if (const UMythicFactionDatabase *FDB = LW->GetFactionDatabase()) {
        UE_LOG(Myth, Warning, TEXT("  Faction DB: %d active / %d max"), FDB->GetActiveFactionCount(), FDB->GetMaxFactions());
    }
    else {
        UE_LOG(Myth, Warning, TEXT("  Faction DB: NOT initialized"));
    }

    // Territory Grid
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
        // Compact behavior flags string
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

    // Cross-reference faction name if DB is available
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

    // Count NPCs (entities with FMythicNPCTag + FMythicIdentityFragment)
    int32 NPCCount = 0;
    {
        FMassEntityQuery NPCQuery(EntityManagerPtr);
        NPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
        NPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        NPCCount = NPCQuery.GetNumMatchingEntities();
    }

    // Count Creatures (entities with FMythicCreatureTag + FMythicCreatureFragment)
    int32 CreatureCount = 0;
    {
        FMassEntityQuery CreatureQuery(EntityManagerPtr);
        CreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
        CreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
        CreatureCount = CreatureQuery.GetNumMatchingEntities();
    }

    // Count Hydrated (Tier 1+ entities)
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
        // Snapshot the settlement under SimulationLock (copy-out) — never hold a live Settlements-map pointer while the
        // sim thread may rehash/mutate it (matches the population spawner's iter-114 fix).
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

    // Snapshot the settlement under SimulationLock (copy-out) — the unlocked reads below (OldFactionName, DisplayName)
    // would otherwise hold a live Settlements-map pointer racing the sim thread's conquest/succession writes.
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

    // Route through the subsystem's TransferSettlement, which holds SimulationLock — calling the registry's directly
    // here (game thread) would race the sim-thread single-writer CausalFabric inside TransferSettlement's event write.
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

// ============================================================================
// LIVING WORLD: EVENT PIPELINE (Phase 4)
// ============================================================================

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

    // Parse the moral axis from string
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

    // Build the action event
    FMythicActionEvent Event;
    Event.Perpetrator = PC->GetPawn();
    Event.ActionTag = FGameplayTag::RequestGameplayTag(FName(*ActionTag), false);
    if (!Event.ActionTag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT(">>> Tag '%s' not registered — event will still submit with invalid tag"), *ActionTag);
    }
    Event.MoralVector.AxisValues[AxisIndex] = MoralValue;
    Event.CategoryFlags = 0x01; // Combat category for testing
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

    // Query hydrated entities near the player
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

    // Find the nearest Tier 0 entity and force-promote it
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

            // Deferred commands for fragment/tag addition
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

// ============================================================================
// LIVING WORLD: PHASE 5 (Cognitive + Social)
// ============================================================================

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

    FString PlayerKey; // local player's canonical party key
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

    // Aim from the pawn (no camera wired on the player pawn): origin at the eyes, direction = pawn facing. This is the
    // same (origin, dir) the future ghost-preview + deploy input will pass to ServerDeployPlaceable.
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
