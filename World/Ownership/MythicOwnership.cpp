
#include "World/Ownership/MythicOwnership.h"

#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/MythicPlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"


namespace MythicTheftCrime {
bool ShouldSubmitTheft(const FMythicOwnership &Ownership, const FGameplayTag &ThiefFactionTag, bool bEnabled) {
    if (!bEnabled) {
        return false;
    }
    if (!Ownership.IsOwned()) {
        return false;
    }
    if (ThiefFactionTag.IsValid() && Ownership.OwnerFactionTag.IsValid() && ThiefFactionTag == Ownership.OwnerFactionTag) {
        return false;
    }
    return true;
}

bool TrySubmitTheft(AActor *Instigator, AActor *OwnedActor, const FMythicOwnership &Ownership) {
    if (!Instigator || !OwnedActor) {
        return false;
    }
    UWorld *World = OwnedActor->GetWorld();
    if (!World) {
        return false;
    }
    if (!OwnedActor->HasAuthority()) {
        return false;
    }

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bEnabled = Settings ? Settings->bOwnershipCrimeEnabled : true;

    const FGameplayTag ThiefFactionTag;

    if (!ShouldSubmitTheft(Ownership, ThiefFactionTag, bEnabled)) {
        return false;
    }

    UMythicActionEventSubsystem *ActionSub = World->GetSubsystem<UMythicActionEventSubsystem>();
    if (!ActionSub) {
        return false;
    }

    APawn *ThiefPawn = Cast<APawn>(Instigator);
    AController *Controller = Cast<AController>(Instigator);
    if (!Controller && ThiefPawn) {
        Controller = ThiefPawn->GetController();
    }
    if (!ThiefPawn && Controller) {
        ThiefPawn = Controller->GetPawn();
    }

    FString PerpPlayerKey;
    if (Controller) {
        if (const AMythicPlayerState *PS = Controller->GetPlayerState<AMythicPlayerState>()) {
            PerpPlayerKey = PS->GetCanonicalPlayerKey();
        }
    }

    FMythicFactionId OwnerFactionId;
    if (Ownership.OwnerFactionTag.IsValid()) {
        if (const UGameInstance *GI = World->GetGameInstance()) {
            if (const UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                if (const UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase()) {
                    OwnerFactionId = FactionDB->FindFactionId(Ownership.OwnerFactionTag);
                }
            }
        }
    }

    FMythicActionEvent Theft;
    Theft.Perpetrator = ThiefPawn ? ThiefPawn : Instigator;
    Theft.VictimFactionOverride = OwnerFactionId;
    Theft.ActionTag = TAG_LIVINGWORLD_ACTION_THEFT_STEAL;
    Theft.CategoryFlags = EMythicEventCategory::Crime;
    Theft.Significance = 0.6f;
    Theft.MoralVector = FMythicMoralSignature::MakeTheftActionMoralVector();
    Theft.PerpPlayerKey = PerpPlayerKey;
    ActionSub->SubmitAction(Theft);
    return true;
}
}


UMythicOwnershipComponent::UMythicOwnershipComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

bool UMythicOwnershipComponent::TrySubmitTheft(AActor *Instigator) {
    return MythicTheftCrime::TrySubmitTheft(Instigator, GetOwner(), Ownership);
}

void UMythicOwnershipComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicOwnershipComponent, Ownership);
}
