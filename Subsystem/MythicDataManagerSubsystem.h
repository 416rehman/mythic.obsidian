
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Itemization/Loot/MythicWorldItem.h"
#include "MythicDataManagerSubsystem.generated.h"

class FSQLiteDatabase;

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicDataManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
//     // Initialize the subsystem
//     virtual void Initialize(FSubsystemCollectionBase& Collection) override;
//
//     // Deinitialize the subsystem
//     virtual void Deinitialize() override;
//
//     // Add an unclaimed item to the database
//     void AddUnclaimedItem(FUniqueNetIdRepl PlayerID, AMythicWorldItem* Item);
//
//     // Get unclaimed items from the database
//     TArray<AMythicWorldItem*> GetUnclaimedItems(FUniqueNetIdRepl PlayerID);
//
//     // Remove an unclaimed item from the database
//     void RemoveUnclaimedItem(FUniqueNetIdRepl PlayerID, AMythicWorldItem* Item);
//
// private:
//     // Connect to the database
//     void ConnectToDatabase();
//
//     // Disconnect from the database
//     void DisconnectFromDatabase();
//
//     // The SQLite database connection
//     FSQLiteDatabase* Database;
};
