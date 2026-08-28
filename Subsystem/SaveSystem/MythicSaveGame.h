#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/CharacterData.h"
#include "World/WorldData.h"
#include "MythicSaveGame.generated.h"

UCLASS()
class MYTHIC_API UMythicSaveGame : public USaveGame {
    GENERATED_BODY()

public:
    /** Sanitized local slot identity shown to Blueprint; mutating it does not rename an already-written save file. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FString SaveSlotName;

    /** Local capture timestamp shown to Blueprint for presentation; it is not authoritative gameplay time. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FDateTime CreationTime;

    /** Integrity checksum computed natively over the serialized payload; Blueprint may inspect but cannot overwrite it. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Data")
    FString DataChecksum;

    /** Character payload populated for character slots; Blueprint may stage data but native validation owns loading. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedCharacterData CharacterData;

    /** Authority-world payload, including harvest lifecycle/outbox state; native validation owns restoration. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedWorldData WorldData;


    void FixupData();
};
