
#include "MythicCemeterySubsystem.h"

#include "MythicGrave.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythCemetery, Log, All);

bool UMythicCemeterySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicCemeterySubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        LivingWorldSubsystem = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    GraveClass = AMythicGrave::StaticClass();

    Config.NotableRoleTags.AddTag(TAG_NPC_ROLE_NOBLE.GetTag());
    Config.NotableRoleTags.AddTag(TAG_NPC_ROLE_MERCHANT.GetTag());
    Config.NotableRoleTags.AddTag(TAG_NPC_ROLE_SOCIALITE.GetTag());

    EpitaphTemplates.Reset();
    {
        FMythicEpitaphTemplate Wild;
        Wild.BodyFormat = FText::FromString(TEXT("Here lies {name}. Fallen on day {day}, remembered still."));
        EpitaphTemplates.Add(Wild);

        FMythicEpitaphTemplate Noble;
        Noble.RoleTag = TAG_NPC_ROLE_NOBLE.GetTag();
        Noble.BodyFormat = FText::FromString(TEXT("Here lies {name}, noble of {faction}. Their line endures beyond day {day}."));
        EpitaphTemplates.Add(Noble);

        FMythicEpitaphTemplate Merchant;
        Merchant.RoleTag = TAG_NPC_ROLE_MERCHANT.GetTag();
        Merchant.BodyFormat = FText::FromString(TEXT("Here lies {name}, {role} of {faction}. The market is quieter since day {day}."));
        EpitaphTemplates.Add(Merchant);

        FMythicEpitaphTemplate Soldier;
        Soldier.RoleTag = TAG_NPC_ROLE_SOLDIER.GetTag();
        Soldier.BodyFormat = FText::FromString(TEXT("Here lies {name}, {role} of {faction}, fallen in the wars around day {day}."));
        EpitaphTemplates.Add(Soldier);

        FMythicEpitaphTemplate Guard;
        Guard.RoleTag = TAG_NPC_ROLE_GUARD.GetTag();
        Guard.BodyFormat = FText::FromString(TEXT("Here lies {name}, {role} of {faction}, who kept the watch until day {day}."));
        EpitaphTemplates.Add(Guard);
    }

    const FString JsonPath = FPaths::ProjectContentDir() / TEXT("Mythic/Death/epitaph_templates.json");
    if (FPaths::FileExists(JsonPath)) {
        FString JsonText;
        TArray<FMythicEpitaphTemplate> Authored;
        if (FFileHelper::LoadFileToString(JsonText, *JsonPath) && ParseEpitaphTemplatesJson(JsonText, Authored)) {
            EpitaphTemplates.Append(Authored);
            UE_LOG(LogMythCemetery, Log, TEXT("Cemetery: loaded %d authored epitaph template(s) from %s"), Authored.Num(), *JsonPath);
        }
    }
}

void UMythicCemeterySubsystem::NotifyDeath(const FMythicCemeteryDeathRecord &Record) {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }
    if (!FMythicCemeteryRules::IsNotableDeath(Record.RoleTag, Record.Significance, Config)) {
        return;
    }

    FName CemeteryKey;
    FVector Anchor;
    ResolveCemeteryAnchor(Record, CemeteryKey, Anchor);

    EnsureCountsSeeded();
    int32 &Count = GraveCounts.FindOrAdd(CemeteryKey);
    if (Count >= Config.MaxGravesPerCemetery) {
        return;
    }

    const FVector SlotOffset = FMythicCemeteryRules::ComputeGraveSlotOffset(Count, Config.GraveSpacing, Config.GravesPerRow);
    const FTransform GraveTransform(FRotator::ZeroRotator, Anchor + SlotOffset);

    FMythicGraveIdentity Identity;
    Identity.SourceNameHash = Record.SourceNameHash;
    Identity.DisplayName = Record.DisplayName;
    Identity.Epitaph = ComposeEpitaph(Record);
    Identity.Faction = Record.Faction;
    Identity.RoleTag = Record.RoleTag;
    Identity.SourceTier = Record.SourceTier;
    Identity.DeathTime = static_cast<float>(Record.DeathTime);
    Identity.KillerNameHash = Record.KillerNameHash;
    Identity.CemeteryKey = CemeteryKey;

    const TSubclassOf<AMythicGrave> SpawnClass = GraveClass ? GraveClass : TSubclassOf<AMythicGrave>(AMythicGrave::StaticClass());
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMythicGrave *Grave = World->SpawnActor<AMythicGrave>(SpawnClass, GraveTransform, SpawnParams);
    if (!Grave) {
        return;
    }
    Grave->ServerInitializeFromDeath(Identity, GraveTransform);
    ++Count;

    UE_LOG(LogMythCemetery, Log, TEXT("Cemetery: grave #%d in '%s' for %s (%s)"), Count, *CemeteryKey.ToString(),
           *Record.DisplayName.ToString(), *Record.RoleTag.ToString());
}

void UMythicCemeterySubsystem::ResolveCemeteryAnchor(const FMythicCemeteryDeathRecord &Record, FName &OutKey, FVector &OutAnchor) const {
    if (LivingWorldSubsystem) {
        FMythicSettlementData Data;
        if (LivingWorldSubsystem->CopySettlementAtCell(Record.HomeCell, Data)) {
            OutKey = Data.SettlementTag.IsValid()
                ? Data.SettlementTag.GetTagName()
                : FName(*FString::Printf(TEXT("Settlement:%d"), Data.SettlementId));

            FVector Origin = Record.DeathLocation;
            if (AMythicSettlement *Actor = LivingWorldSubsystem->GetSettlementActorSafe(Data.SettlementId)) {
                Origin = Actor->GetActorLocation();
            }
            else if (const UMythicTerritoryGrid *Grid = LivingWorldSubsystem->GetTerritoryGrid()) {
                Origin = Grid->CellToWorld(Data.CenterCell);
            }
            OutAnchor = Origin + Config.CemeteryAnchorOffset;
            return;
        }
    }

    FMythicCellCoord DeathCell = Record.HomeCell;
    FVector Origin = Record.DeathLocation;
    if (LivingWorldSubsystem) {
        if (const UMythicTerritoryGrid *Grid = LivingWorldSubsystem->GetTerritoryGrid()) {
            DeathCell = Grid->WorldToCell(Record.DeathLocation);
            Origin = Grid->CellToWorld(DeathCell);
        }
    }
    OutKey = FName(*FString::Printf(TEXT("Wild:%d,%d"), DeathCell.X, DeathCell.Y));
    OutAnchor = Origin;
}

FText UMythicCemeterySubsystem::ComposeEpitaph(const FMythicCemeteryDeathRecord &Record) const {
    const int32 Day = FMythicCemeteryRules::WorldDayForSeconds(Record.DeathTime, Config.SecondsPerWorldDay);
    const FName NameKey(*Record.DisplayName.ToString());
    const int32 Index = FMythicEpitaph::SelectEpitaphTemplate(Record.RoleTag, Record.Faction, EpitaphTemplates);
    if (EpitaphTemplates.IsValidIndex(Index)) {
        return FMythicEpitaph::Compose(EpitaphTemplates[Index], NameKey, Record.RoleTag, Record.Faction, Day);
    }
    return FText::FromString(FString::Printf(TEXT("Here lies %s. Fallen on day %d."), *Record.DisplayName.ToString(), Day));
}

void UMythicCemeterySubsystem::EnsureCountsSeeded() {
    if (bCountsSeeded) {
        return;
    }
    bCountsSeeded = true;

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    for (TActorIterator<AMythicGrave> It(World); It; ++It) {
        const AMythicGrave *Grave = *It;
        if (!IsValid(Grave)) {
            continue;
        }
        const FName Key = Grave->GetCemeteryKey();
        if (!Key.IsNone()) {
            ++GraveCounts.FindOrAdd(Key);
        }
    }
}

bool UMythicCemeterySubsystem::ParseEpitaphTemplatesJson(const FString &JsonText, TArray<FMythicEpitaphTemplate> &Out) {
    Out.Reset();

    TArray<TSharedPtr<FJsonValue>> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root)) {
        return false;
    }

    for (const TSharedPtr<FJsonValue> &Value : Root) {
        const TSharedPtr<FJsonObject> *ObjPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid()) {
            continue;
        }
        const TSharedPtr<FJsonObject> &Obj = *ObjPtr;

        FMythicEpitaphTemplate Template;
        FString RoleStr;
        if (Obj->TryGetStringField(TEXT("role"), RoleStr) && !RoleStr.IsEmpty()) {
            Template.RoleTag = FGameplayTag::RequestGameplayTag(FName(*RoleStr), false);
        }
        FString FactionStr;
        if (Obj->TryGetStringField(TEXT("faction"), FactionStr) && !FactionStr.IsEmpty()) {
            Template.Faction = FGameplayTag::RequestGameplayTag(FName(*FactionStr), false);
        }
        FString Body;
        Obj->TryGetStringField(TEXT("body"), Body);
        Template.BodyFormat = FText::FromString(Body);

        Out.Add(Template);
    }

    return Out.Num() > 0;
}
