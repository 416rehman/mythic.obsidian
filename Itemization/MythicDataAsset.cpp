#include "MythicDataAsset.h"
#include "DataConfig/DcEnv.h"
#include "DataConfig/Automation/DcAutomationUtils.h"
#include "DataConfig/Json/DcJsonWriter.h"
#include "DataConfig/Json/DcJsonReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "Mythic/Mythic.h" // Assuming "Mythic.h" defines the "Myth" log category
#include "AssetRegistry/AssetRegistryModule.h"
#include "Windows/WindowsPlatformApplicationMisc.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/Package.h"

void UMythicDataAsset::CopyJSONToClipboard() const {
    FString OutJsonString;
    ExportAsJSONString(OutJsonString);

    FPlatformApplicationMisc::ClipboardCopy(*OutJsonString);

    UE_LOG(Myth, Log, TEXT("Copied JSON to clipboard"));
}

void UMythicDataAsset::ExportToJSON() const {
    FString OutJsonString;
    ExportAsJSONString(OutJsonString);

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

void UMythicDataAsset::ExportAsJSONString(FString &OutJsonString) const {
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

void UMythicDataAsset::ImportFromJSON() {
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

    ImportFromJSONString(FileContent);
}

void UMythicDataAsset::ImportFromClipboard() {
    FString ClipboardContent;
    FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

    if (ClipboardContent.IsEmpty()) {
        UE_LOG(Myth, Warning, TEXT("Clipboard is empty"));
        return;
    }

    ImportFromJSONString(ClipboardContent);
}

void UMythicDataAsset::ImportFromJSONString(const FString &JsonString) {
    DcStartUp(EDcInitializeAction::Minimal);

    FDcDeserializer JSONDeserializer;
    DcSetupJsonDeserializeHandlers(JSONDeserializer);

    FDcJsonReader Reader(JsonString);
    FDcPropertyWriter Writer(FDcPropertyDatum(this));

    FDcDeserializeContext Ctx;
    Ctx.Reader = &Reader;
    Ctx.Writer = &Writer;
    Ctx.Deserializer = &JSONDeserializer;
    Ctx.Objects.Add(this);

    if (Ctx.Prepare().Ok() && Ctx.Deserializer->Deserialize(Ctx).Ok()) {
        UE_LOG(Myth, Log, TEXT("Successfully imported from JSON"));
        MarkPackageDirty();
    }
    else {
        UE_LOG(Myth, Error, TEXT("Failed to deserialize from JSON"));
    }

    DcShutDown();
}

void UMythicDataAssetJSONImportLibrary::ImportDataAssetsFromFolder(TSubclassOf<UMythicDataAsset> AssetClass, FString PackagePath) {
    if (!AssetClass) {
        UE_LOG(Myth, Error, TEXT("ImportDataAssetsFromFolder: Invalid AssetClass"));
        return;
    }

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

        FString AssetName = FPaths::GetBaseFilename(FileName);
        // Ensure PackagePath ends with / if not empty
        if (!PackagePath.IsEmpty() && !PackagePath.EndsWith("/")) {
            PackagePath += "/";
        }
        FString FullPackageName = PackagePath + AssetName;
        UPackage *Package = CreatePackage(*FullPackageName);

        UMythicDataAsset *NewAsset = NewObject<UMythicDataAsset>(Package, AssetClass, *AssetName, RF_Public | RF_Standalone | RF_MarkAsRootSet);

        DcStartUp(EDcInitializeAction::Minimal);
        FDcDeserializer JSONDeserializer;
        DcSetupJsonDeserializeHandlers(JSONDeserializer);

        FDcJsonReader Reader(FileContent);
        FDcPropertyWriter Writer((FDcPropertyDatum(NewAsset)));

        FDcDeserializeContext Ctx;
        Ctx.Reader = &Reader;
        Ctx.Writer = &Writer;
        Ctx.Deserializer = &JSONDeserializer;
        Ctx.Objects.Add(NewAsset);

        if (Ctx.Prepare().Ok() && Ctx.Deserializer->Deserialize(Ctx).Ok()) {
            UE_LOG(Myth, Log, TEXT("Imported %s"), *AssetName);
            FAssetRegistryModule::AssetCreated(NewAsset);
            NewAsset->MarkPackageDirty();
        }
        else {
            UE_LOG(Myth, Error, TEXT("Failed to deserialize: %s"), *FileName);
        }

        DcShutDown();
    }

    UE_LOG(Myth, Log, TEXT("Batch import completed."));
}

#endif
