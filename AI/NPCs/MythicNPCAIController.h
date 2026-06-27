//

#pragma once

#include "CoreMinimal.h"
#include "AI/NPCs/MythicAIController.h"
#include "MythicNPCAIController.generated.h"

/**
 * Concrete, spawnable AI controller for embodied Living-World NPCs.
 *
 * AMythicAIController is UCLASS(Abstract) — it was authored to be subclassed by a combat Blueprint and cannot be
 * instantiated directly. The MASS embodiment path (UMythicActorSpawnProcessor) spawns a plain AMythicNPCCharacter,
 * which needs a NON-abstract controller class to auto-possess. This subclass adds no new state; it exists solely to
 * make the base controller's perception / idle-patrol / companion-follow logic instantiable for code-spawned NPCs.
 * A combat NPC Blueprint may still subclass AMythicAIController and set it as its pawn's AIControllerClass for
 * authored attack behavior.
 */
UCLASS()
class MYTHIC_API AMythicNPCAIController : public AMythicAIController {
    GENERATED_BODY()
};
