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
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyDefinition.h"
#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#include "Mythic/Mythic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

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

    UProficiencyComponent *ProfComp = PC->FindComponentByClass<UProficiencyComponent>();
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

    UProficiencyComponent *ProfComp = PC->FindComponentByClass<UProficiencyComponent>();
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
