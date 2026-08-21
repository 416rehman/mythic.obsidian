#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"

class APlayerController;
struct FMythicSecretDef;

struct MYTHIC_API FMythicSecretReveal {
    static bool TryRevealSecret(APlayerController *PC, const FMythicSecretDef &Def, const FVector &RevealLocation = FVector::ZeroVector);
};
