#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MythicSaveableActor.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMythicSaveableActor : public UInterface {
    GENERATED_BODY()
};

class MYTHIC_API IMythicSaveableActor {
    GENERATED_BODY()

public:
    virtual FString GetSaveableActorId() const {
        if (const AActor *Actor = Cast<AActor>(this)) {
            return Actor->GetPathName();
        }
        return FString();
    }

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) {
    }

    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) {
    }
};
