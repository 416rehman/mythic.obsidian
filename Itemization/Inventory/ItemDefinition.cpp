#include "ItemDefinition.h"
#include  "DataConfig/DcEnv.h"
#include "DataConfig/Automation/DcAutomationUtils.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/Json/DcJsonReader.h"

#include "Misc/DataValidation.h"

//////////////////
/// VALIDATION ///
//////////////////
#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "Mythic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/ObjectSaveContext.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

void UItemDefinition::PostLoad() {
    Super::PostLoad();

    // Remove any null fragments
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

void UItemDefinition::PreSave(FObjectPreSaveContext SaveContext) {
    Super::PreSave(SaveContext);

    // Remove any null fragments before saving
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

EDataValidationResult UItemDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    bool bHasNullFragments = false;
    for (int32 Index = 0; Index < Fragments.Num(); ++Index) {
        if (!Fragments[Index]) {
            Context.AddError(FText::Format(
                NSLOCTEXT("ItemDefinition", "NullFragmentError", "Null fragment at index {0}"),
                FText::AsNumber(Index)));
            bHasNullFragments = true;
        }
    }

    bool bHasValidType = false;
    // makes sure itemtype is a child of itemization.type
    if (ItemType.IsValid()) {
        FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Itemization.Type"));
        if (ItemType.MatchesTag(ParentTag)) {
            bHasValidType = true;
        }
    }

    bool result = bHasValidType && !bHasNullFragments;
    return result ? EDataValidationResult::Invalid : Result;
}

void UItemDefinition::CopyJSONToClipboard() const {
    FString OutJsonString;
    ExportAsJSONString(OutJsonString); // See helper below

    // Copy to clipboard
    FPlatformApplicationMisc::ClipboardCopy(*OutJsonString);

    UE_LOG(Myth, Log, TEXT("Copied JSON to clipboard"));
}

void UItemDefinition::ExportToJSON() const {
    FString OutJsonString;
    ExportAsJSONString(OutJsonString); // See helper below

    // File save dialog
    FString DefaultPath = FPaths::ProjectDir();
    FString FileName = GetName() + TEXT(".json");
    TArray<FString> OutFiles;

    IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform) {
        bool bSaved = DesktopPlatform->SaveFileDialog(
            nullptr,
            TEXT("Save JSON File"),
            DefaultPath,
            FileName,
            TEXT("JSON Files (*.json)|*.json"),
            EFileDialogFlags::None,
            OutFiles
            );

        if (bSaved && OutFiles.Num() > 0) {
            FString SavePath = OutFiles[0];
            if (FFileHelper::SaveStringToFile(OutJsonString, *SavePath)) {
                UE_LOG(Myth, Log, TEXT("Saved JSON to %s"), *SavePath);
            }
            else {
                UE_LOG(Myth, Error, TEXT("Failed to save file: %s"), *SavePath);
            }
        }
    }
}

void UItemDefinition::ExportAsJSONString(FString &OutJsonString) const {
    DcStartUp(EDcInitializeAction::Minimal);

    FDcPropertyDatum Datum((UObject *)this);

    FDcSerializer JSONSerializer;
    DcSetupJsonSerializeHandlers(JSONSerializer);

    FDcPropertyReader Reader(Datum);
    FDcJsonWriter Writer;

    FDcSerializeContext Ctx;
    Ctx.Reader = &Reader;
    Ctx.Writer = &Writer;
    Ctx.Serializer = &JSONSerializer;

    if (Ctx.Prepare().Ok() && Ctx.Serializer->Serialize(Ctx).Ok()) {
        OutJsonString = Writer.Sb.ToString();
    }
    else {
        OutJsonString.Empty();
        UE_LOG(Myth, Error, TEXT("Failed to serialize to JSON"));
    }

    DcShutDown();
}

void UItemDefinition::ImportFromJSON() {
    // Open file dialog to select JSON file
    TArray<FString> OutFiles;
    auto FileDialog = FDesktopPlatformModule::Get()->OpenFileDialog(
        nullptr,
        TEXT("Select JSON File"),
        TEXT(""),
        TEXT(""),
        TEXT("JSON Files (*.json)|*.json"),
        EFileDialogFlags::None,
        OutFiles
        );
    if (!FileDialog || OutFiles.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("No file selected"));
        return;
    }
    FString SelectedFile = OutFiles[0];
    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *SelectedFile)) {
        UE_LOG(Myth, Error, TEXT("Failed to load file: %s"), *SelectedFile);
        return;
    }

    DcStartUp(EDcInitializeAction::Minimal);

    // Deserializer
    FDcDeserializer JSONDeserializer;
    DcSetupJsonDeserializeHandlers(JSONDeserializer);

    // Reader and writer
    FDcJsonReader Reader(FileContent);
    FDcPropertyWriter Writer(FDcPropertyDatum(this));

    // Context
    FDcDeserializeContext Ctx;
    Ctx.Reader = &Reader;
    Ctx.Writer = &Writer;
    Ctx.Deserializer = &JSONDeserializer;
    Ctx.Objects.Add(this);

    if (Ctx.Prepare().Ok() && Ctx.Deserializer->Deserialize(Ctx).Ok()) {
        UE_LOG(Myth, Log, TEXT("Successfully imported from JSON"));
    }
    else {
        // Handle error
        UE_LOG(Myth, Error, TEXT("Failed to deserialize from JSON"));
    }

    DcShutDown();
}

void UItemDefinitionJSONImportLibrary::ImportItemDefinitionsFromFolder() {
    IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) {
        return;
    }

    FString FolderPath;
    const void *ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

    if (!DesktopPlatform->OpenDirectoryDialog(
        ParentWindowHandle,
        TEXT("Select JSON Folder"),
        TEXT(""),
        FolderPath)) {
        UE_LOG(Myth, Warning, TEXT("No folder selected"));
        return;
    }

    // Get all JSON files in folder
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *FolderPath, TEXT("*.json"));

    if (Files.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("No JSON files found in %s"), *FolderPath);
        return;
    }

    for (const FString &FileName : Files) {
        FString FilePath = FPaths::Combine(FolderPath, FileName);
        FString FileContent;
        if (!FFileHelper::LoadFileToString(FileContent, *FilePath)) {
            UE_LOG(Myth, Error, TEXT("Failed to load file: %s"), *FilePath);
            continue;
        }

        // Create new asset name
        FString AssetName = FPaths::GetBaseFilename(FileName);
        FString PackageName = TEXT("/Game/Mythic/Itemization/ItemDefinitions/Weapons/") + AssetName;
        UPackage *Package = CreatePackage(*PackageName);

        // Create the asset
        UItemDefinition *NewItem = NewObject<UItemDefinition>(Package, *AssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

        // Deserialize JSON into the new asset
        DcStartUp(EDcInitializeAction::Minimal);
        FDcDeserializer JSONDeserializer;
        DcSetupJsonDeserializeHandlers(JSONDeserializer);

        FDcJsonReader Reader(FileContent);
        FDcPropertyWriter Writer(FDcPropertyDatum((UObject *)NewItem));

        FDcDeserializeContext Ctx;
        Ctx.Reader = &Reader;
        Ctx.Writer = &Writer;
        Ctx.Deserializer = &JSONDeserializer;
        Ctx.Objects.Add(NewItem);

        if (Ctx.Prepare().Ok() && Ctx.Deserializer->Deserialize(Ctx).Ok()) {
            UE_LOG(Myth, Log, TEXT("Imported %s"), *AssetName);

            // Mark asset dirty so UE saves it
            FAssetRegistryModule::AssetCreated(NewItem);
            NewItem->MarkPackageDirty();
        }
        else {
            UE_LOG(Myth, Error, TEXT("Failed to deserialize: %s"), *FileName);
        }

        DcShutDown();
    }

    UE_LOG(Myth, Log, TEXT("Batch import completed."));
}
#endif
