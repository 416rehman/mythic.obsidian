
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Inventory/WorldItem.h"
#include "DataManagerSubsystem.generated.h"

class FSQLiteDatabase;

/**
 * 
 */
UCLASS()
class MYTHIC_API UDataManagerSubsystem : public UGameInstanceSubsystem
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
//     void AddUnclaimedItem(FUniqueNetIdRepl PlayerID, AWorldItem* Item);
//
//     // Get unclaimed items from the database
//     TArray<AWorldItem*> GetUnclaimedItems(FUniqueNetIdRepl PlayerID);
//
//     // Remove an unclaimed item from the database
//     void RemoveUnclaimedItem(FUniqueNetIdRepl PlayerID, AWorldItem* Item);
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
