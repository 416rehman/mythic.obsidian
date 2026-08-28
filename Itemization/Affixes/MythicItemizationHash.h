#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"

namespace MythicItemizationHash {
MYTHIC_API bool Sha256(TConstArrayView<uint8> Bytes, FSHA256Signature &OutSignature);
}
