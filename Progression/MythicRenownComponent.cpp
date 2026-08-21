
#include "GAS/Progression/MythicRenownComponent.h"

#include "GAS/Progression/MythicRenownRules.h"
#include "Progression/MythicTags_MetaProgression.h"

#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Player/MythicFactionStandingComponent.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Settings/MythicDeveloperSettings.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "Mythic/Mythic.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "GameplayTagsManager.h"
#include "Net/UnrealNetwork.h"

namespace {
    constexpr float GDefaultRenownThresholds[7] = {-6000.0f, -3000.0f, 0.0f, 3000.0f, 9000.0f, 21000.0f, 42000.0f};
    constexpr float GDefaultVendorDiscounts[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.05f, 0.10f, 0.15f, 0.20f};

    FGameplayTag FactionRootTag() {
        static const FGameplayTag Root = FGameplayTag::RequestGameplayTag(FName(TEXT("Faction")),false);
        return Root;
    }

    bool TierRewardIsSet(const FRewardsToGive &R) {
        return R.XPReward || R.ItemReward || R.LootReward || R.AbilityReward || R.AttributeReward
            || R.RenownReward;
    }
}

UMythicRenownComponent::UMythicRenownComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicRenownComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicRenownComponent, RenownEntries, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRenownComponent, GlobalRenown, COND_OwnerOnly);
}


const UMythicRenownTierTable *UMythicRenownComponent::ResolveTierTable() const {
    UMythicRenownComponent *Self = const_cast<UMythicRenownComponent *>(this);
    if (Self->bTableResolved) {
        return Self->ResolvedTierTable;
    }
    Self->bTableResolved = true;
    if (!TierTable.IsNull()) {
        Self->ResolvedTierTable = TierTable.LoadSynchronous();
    }
    if (!Self->ResolvedTierTable) {
        if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
            if (!Settings->DefaultRenownTierTable.IsNull()) {
                Self->ResolvedTierTable = Settings->DefaultRenownTierTable.LoadSynchronous();
            }
        }
    }
    return Self->ResolvedTierTable;
}

TConstArrayView<float> UMythicRenownComponent::GetThresholds() const {
    const UMythicRenownTierTable *Table = ResolveTierTable();
    if (Table && Table->Thresholds.Num() > 0) {
        return Table->Thresholds;
    }
    return MakeArrayView(GDefaultRenownThresholds, UE_ARRAY_COUNT(GDefaultRenownThresholds));
}

TConstArrayView<float> UMythicRenownComponent::GetVendorDiscounts() const {
    const UMythicRenownTierTable *Table = ResolveTierTable();
    if (Table && Table->VendorDiscounts.Num() > 0) {
        return Table->VendorDiscounts;
    }
    return MakeArrayView(GDefaultVendorDiscounts, UE_ARRAY_COUNT(GDefaultVendorDiscounts));
}

UMythicNarrativeStateComponent *UMythicRenownComponent::ResolveNarrative() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetNarrativeState();
    }
    return nullptr;
}

UMythicFactionStandingComponent *UMythicRenownComponent::ResolveFactionStanding() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetFactionStanding();
    }
    return nullptr;
}

APlayerController *UMythicRenownComponent::ResolvePC() const {
    if (const APlayerState *PS = Cast<APlayerState>(GetOwner())) {
        return PS->GetPlayerController();
    }
    return nullptr;
}


float UMythicRenownComponent::GetRenown(FGameplayTag Scope) const {
    if (Scope == RENOWN_SCOPE_GLOBAL) {
        return GlobalRenown;
    }
    for (const FMythicRenownEntry &Entry : RenownEntries) {
        if (Entry.ScopeTag == Scope) {
            return Entry.Value;
        }
    }
    return 0.0f;
}

EMythicRenownTier UMythicRenownComponent::EffectiveTier(const FGameplayTag &Scope, float Value) const {
    const TConstArrayView<float> Thresholds = GetThresholds();

    const FGameplayTag FactionRoot = FactionRootTag();
    if (FactionRoot.IsValid() && Scope.MatchesTag(FactionRoot)) {
        if (const UMythicFactionStandingComponent *Standing = ResolveFactionStanding()) {
            const UGameInstance *GI = GetOwner() ? GetOwner()->GetGameInstance() : nullptr;
            const UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
            const UMythicFactionDatabase *FDB = LWS ? LWS->GetFactionDatabase() : nullptr;
            if (FDB) {
                const FMythicFactionId FactionId = FDB->FindFactionId(Scope);
                if (FactionId.IsValid() &&
                    Standing->TierForStanding(Standing->GetStanding(FactionId)) == EMythicStandingTier::Hostile) {
                    const UMythicRenownTierTable *Table = ResolveTierTable();
                    const EMythicRenownTier Cap = Table ? Table->HostileStandingTierCap : EMythicRenownTier::Neutral;
                    return FMythicRenownRules::ClampToMaxTier(Value, Cap, Thresholds);
                }
            }
        }
    }
    return FMythicRenownRules::TierForValue(Value, Thresholds);
}

EMythicRenownTier UMythicRenownComponent::GetTier(FGameplayTag Scope) const {
    return EffectiveTier(Scope, GetRenown(Scope));
}

EMythicRenownTier UMythicRenownComponent::GetGlobalTier() const {
    return FMythicRenownRules::TierForValue(GlobalRenown, GetThresholds());
}

float UMythicRenownComponent::GetVendorDiscount(FGameplayTag Scope) const {
    return FMythicRenownRules::VendorDiscountForTier(GetTier(Scope), GetVendorDiscounts());
}


FGameplayTag UMythicRenownComponent::MakeTierMirrorTag(const FGameplayTag &Scope, EMythicRenownTier Tier) {
    const FString Full = Scope.ToString();
    int32 LastDot = INDEX_NONE;
    Full.FindLastChar(TEXT('.'), LastDot);
    const FString Leaf = LastDot != INDEX_NONE ? Full.Mid(LastDot + 1) : Full;
    const FString MirrorName = FString::Printf(TEXT("Renown.%s.%s"), *Leaf, FMythicRenownRules::TierName(Tier));
    return FGameplayTag::RequestGameplayTag(FName(*MirrorName),false);
}

void UMythicRenownComponent::HandleTierChange(const FGameplayTag &Scope, EMythicRenownTier OldTier, EMythicRenownTier NewTier) {
    if (OldTier == NewTier) {
        return;
    }

    OnRenownTierChanged.Broadcast(Scope, NewTier);
    UE_LOG(Myth, Log, TEXT("RenownComponent: %s tier %s -> %s"), *Scope.ToString(),
           FMythicRenownRules::TierName(OldTier), FMythicRenownRules::TierName(NewTier));

    if (NewTier > OldTier) {
        UMythicNarrativeStateComponent *Narrative = ResolveNarrative();
        const UMythicRenownTierTable *Table = ResolveTierTable();

        for (int32 T = static_cast<int32>(OldTier) + 1; T <= static_cast<int32>(NewTier); ++T) {
            const EMythicRenownTier Tier = static_cast<EMythicRenownTier>(T);
            const FGameplayTag Mirror = MakeTierMirrorTag(Scope, Tier);
            const bool bFirstReach = Mirror.IsValid() && Narrative && !Narrative->HasStoryTag(Mirror);

            if (Narrative) {
                if (Mirror.IsValid()) {
                    Narrative->ServerSetStoryTag(Mirror);
                }
                else {
                    UE_LOG(Myth, Warning,
                           TEXT("RenownComponent: tier mirror tag 'Renown.*.%s' for scope %s is not registered — author it "
                               "in the gameplay-tag table so unlock rules/titles can gate on this crossing."),
                           FMythicRenownRules::TierName(Tier), *Scope.ToString());
                }
            }

            if (Table && Table->TierPayloads.IsValidIndex(T)) {
                const FMythicRenownTierPayload &Payload = Table->TierPayloads[T];
                if (Narrative) {
                    for (const FGameplayTag &Extra : Payload.UnlockStoryTags) {
                        if (Extra.IsValid()) {
                            Narrative->ServerSetStoryTag(Extra);
                        }
                    }
                }
                if (!bIsRestoring && bFirstReach && TierRewardIsSet(Payload.Rewards)) {
                    if (APlayerController *PC = ResolvePC()) {
                        Payload.Rewards.Give(PC);
                    }
                }
            }
        }
    }
}

void UMythicRenownComponent::ServerGrantRenown(FGameplayTag Scope, float Amount) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    if (!Scope.IsValid() || Amount == 0.0f) {
        return;
    }

    const bool bGlobalOnly = Scope == RENOWN_SCOPE_GLOBAL;

    EMythicRenownTier ScopeOldTier = EMythicRenownTier::Hated, ScopeNewTier = EMythicRenownTier::Hated;
    if (!bGlobalOnly) {
        int32 Index = INDEX_NONE;
        for (int32 i = 0; i < RenownEntries.Num(); ++i) {
            if (RenownEntries[i].ScopeTag == Scope) {
                Index = i;
                break;
            }
        }
        if (Index == INDEX_NONE) {
            Index = RenownEntries.Add(FMythicRenownEntry(Scope, 0.0f));
        }
        const float OldValue = RenownEntries[Index].Value;
        const float NewValue = OldValue + Amount;
        RenownEntries[Index].Value = NewValue;
        ScopeOldTier = EffectiveTier(Scope, OldValue);
        ScopeNewTier = EffectiveTier(Scope, NewValue);
    }

    const float OldGlobal = GlobalRenown;
    GlobalRenown = OldGlobal + Amount;
    const TConstArrayView<float> Thresholds = GetThresholds();
    const EMythicRenownTier GlobalOldTier = FMythicRenownRules::TierForValue(OldGlobal, Thresholds);
    const EMythicRenownTier GlobalNewTier = FMythicRenownRules::TierForValue(GlobalRenown, Thresholds);

    if (!bGlobalOnly) {
        HandleTierChange(Scope, ScopeOldTier, ScopeNewTier);
    }
    HandleTierChange(RENOWN_SCOPE_GLOBAL, GlobalOldTier, GlobalNewTier);
}


void UMythicRenownComponent::RestoreRenown(const TArray<FMythicRenownEntry> &SavedEntries, float SavedGlobal) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    bIsRestoring = true;
    RenownEntries = SavedEntries;
    GlobalRenown = SavedGlobal;
    bIsRestoring = false;
}
