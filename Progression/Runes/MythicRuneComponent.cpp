
#include "MythicRuneComponent.h"

#include "MythicRuneDefinition.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Mythic/Mythic.h"
#include "Mythic/Narrative/MythicNarrativeStateComponent.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Progression/MythicAchievementComponent.h"
#include "Mythic/Progression/MythicUnlockComponent.h"
#include "Net/UnrealNetwork.h"

void FMythicRuneHudStateArray::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters & /*Parameters*/) {
    if (Owner) {
        Owner->HandleHudStatesReceived();
    }
}

UMythicRuneComponent::UMythicRuneComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    HudStates.SetOwner(this);
}

void UMythicRuneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, EquippedRunes, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, UnlockedSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, HudStates, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRuneComponent, RuneRolls, COND_OwnerOnly);
}

void UMythicRuneComponent::BeginPlay() {
    Super::BeginPlay();
    HudStates.SetOwner(this);

    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        SizeSlots();
    }
}

void UMythicRuneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    const AActor *Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) {
        for (int32 SlotIndex = 0; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
            ClearSlotAbility(SlotIndex);
        }
    }
    Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent *UMythicRuneComponent::ResolveASC() const {
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner())) {
        return ASI->GetAbilitySystemComponent();
    }
    return nullptr;
}

void UMythicRuneComponent::SizeSlots() {
    const int32 Slots = FMath::Max(MaxSlots, 1);
    // Lowering MaxSlots drops sockets: hand their abilities back before the entries go.
    for (int32 SlotIndex = Slots; SlotIndex < GrantedHandles.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
    }
    if (EquippedRunes.Num() != Slots) {
        EquippedRunes.SetNum(Slots);
    }
    if (GrantedHandles.Num() != Slots) {
        GrantedHandles.SetNum(Slots);
    }
    const int32 Dropped = HudStates.Items.RemoveAll([Slots](const FMythicRuneHudStateItem &Item) { return Item.SlotIndex >= Slots; });
    if (Dropped > 0) {
        HudStates.MarkArrayDirty();
        OnRuneHudStateChanged.Broadcast();
    }
}

FMythicRuneHudStateItem *UMythicRuneComponent::FindHudItem(int32 SlotIndex) {
    return HudStates.Items.FindByPredicate([SlotIndex](const FMythicRuneHudStateItem &Item) { return Item.SlotIndex == SlotIndex; });
}

const FMythicRuneHudStateItem *UMythicRuneComponent::FindHudItem(int32 SlotIndex) const {
    return HudStates.Items.FindByPredicate([SlotIndex](const FMythicRuneHudStateItem &Item) { return Item.SlotIndex == SlotIndex; });
}

bool UMythicRuneComponent::ClearHudItem(int32 SlotIndex) {
    const int32 Dropped = HudStates.Items.RemoveAll([SlotIndex](const FMythicRuneHudStateItem &Item) { return Item.SlotIndex == SlotIndex; });
    if (Dropped == 0) {
        return false;
    }
    HudStates.MarkArrayDirty();
    return true;
}

void UMythicRuneComponent::HandleHudStatesReceived() {
    OnRuneHudStateChanged.Broadcast();
}

double UMythicRuneComponent::GetServerWorldTimeSeconds() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 0.0;
    }
    if (const AGameStateBase *GameState = World->GetGameState()) {
        return static_cast<double>(GameState->GetServerWorldTimeSeconds());
    }
    return World->GetTimeSeconds();
}

int32 UMythicRuneComponent::FindSlotOfRune(const UMythicRuneDefinition *Rune) const {
    if (!Rune) {
        return INDEX_NONE;
    }
    const FSoftObjectPath RunePath(Rune);
    for (int32 SlotIndex = 0; SlotIndex < EquippedRunes.Num(); SlotIndex++) {
        if (EquippedRunes[SlotIndex].ToSoftObjectPath() == RunePath) {
            return SlotIndex;
        }
    }
    return INDEX_NONE;
}

const FMythicRuneRollSet *UMythicRuneComponent::FindRollSet(const UMythicRuneDefinition *Rune) const {
    if (!Rune) {
        return nullptr;
    }
    // Compare by path: the owning client reads a replicated set whose asset may not be loaded.
    const FSoftObjectPath RunePath(Rune);
    return RuneRolls.FindByPredicate([&RunePath](const FMythicRuneRollSet &Set) { return Set.Rune.ToSoftObjectPath() == RunePath; });
}

bool UMythicRuneComponent::GetRolledRuneValue(const UMythicRuneDefinition *Rune, FGameplayTag Parameter, float &OutValue) const {
    OutValue = 0.0f;
    const FMythicRuneRollSet *Set = FindRollSet(Rune);
    const FMythicRuneRollValue *Value = Set ? Set->Find(Parameter) : nullptr;
    if (!Value) {
        return false;
    }
    OutValue = Value->Value;
    return true;
}

bool UMythicRuneComponent::EnsureRuneRolled(const UMythicRuneDefinition *Rune) {
    if (!Rune || Rune->Parameters.Num() == 0 || FindRollSet(Rune)) {
        return false;
    }
    FMythicRuneRollSet &Set = RuneRolls.AddDefaulted_GetRef();
    Set.Rune = TSoftObjectPtr<UMythicRuneDefinition>(FSoftObjectPath(Rune));
    Set.Values.Reserve(Rune->Parameters.Num());
    for (const TPair<FGameplayTag, FRollDefinition> &Param : Rune->Parameters) {
        if (!Param.Key.IsValid()) {
            continue;
        }
        FMythicRuneRollValue &Value = Set.Values.AddDefaulted_GetRef();
        Value.Parameter = Param.Key;
        Value.Value = FMath::FRandRange(Param.Value.Min, Param.Value.Max);
        if (Param.Value.bWholeNumber) {
            Value.Value = FMath::RoundToFloat(Value.Value);
        }
        UE_LOG(Myth, Log, TEXT("Runes: %s rolled %s = %g [%g-%g] for %s."), *GetNameSafe(GetOwner()), *Param.Key.ToString(),
               Value.Value, Param.Value.Min, Param.Value.Max, *GetNameSafe(Rune));
    }
    return true;
}

void UMythicRuneComponent::RestoreRuneRolls(const TArray<FMythicRuneRollSet> &SavedRolls) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    RuneRolls.Reset();
    for (const FMythicRuneRollSet &Saved : SavedRolls) {
        const FSoftObjectPath SavedPath = Saved.Rune.ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped a saved roll set that names no rune."));
            continue;
        }
        if (RuneRolls.ContainsByPredicate([&SavedPath](const FMythicRuneRollSet &Set) { return Set.Rune.ToSoftObjectPath() == SavedPath; })) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped a second saved roll set for '%s'."), *SavedPath.ToString());
            continue;
        }
        FMythicRuneRollSet &Set = RuneRolls.AddDefaulted_GetRef();
        Set.Rune = Saved.Rune;
        for (const FMythicRuneRollValue &Value : Saved.Values) {
            if (!Value.Parameter.IsValid() || !FMath::IsFinite(Value.Value)) {
                UE_LOG(Myth, Warning, TEXT("Runes: dropped saved roll '%s' = %g on '%s'."), *Value.Parameter.ToString(), Value.Value,
                       *SavedPath.ToString());
                continue;
            }
            if (!Set.Find(Value.Parameter)) {
                Set.Values.Add(Value);
            }
        }
    }
    OnRuneRollsChanged.Broadcast();
}

EMythicRuneHudState UMythicRuneComponent::GetRuneHudState(int32 SlotIndex) const {
    const FMythicRuneHudStateItem *Item = FindHudItem(SlotIndex);
    return Item ? Item->State : EMythicRuneHudState::Hidden;
}

float UMythicRuneComponent::GetRuneHudRemainingSeconds(int32 SlotIndex) const {
    const FMythicRuneHudStateItem *Item = FindHudItem(SlotIndex);
    if (!Item || Item->ServerEndTimeSeconds <= 0.0) {
        return 0.f;
    }
    return static_cast<float>(FMath::Max(0.0, Item->ServerEndTimeSeconds - GetServerWorldTimeSeconds()));
}

void UMythicRuneComponent::SetRuneHudState(int32 SlotIndex, EMythicRuneHudState State, float DurationSeconds, int32 Stacks) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (SlotIndex < 0 || SlotIndex >= FMath::Max(MaxSlots, 1)) {
        UE_LOG(Myth, Warning, TEXT("Runes: HUD state for slot %d ignored — %s has %d sockets."), SlotIndex, *GetNameSafe(Owner),
               MaxSlots);
        return;
    }
    if (State == EMythicRuneHudState::Hidden) {
        if (ClearHudItem(SlotIndex)) {
            OnRuneHudStateChanged.Broadcast();
        }
        return;
    }

    const double Now = GetServerWorldTimeSeconds();
    const double End = DurationSeconds > 0.f ? Now + DurationSeconds : 0.0;
    const uint8 StackCount = static_cast<uint8>(FMath::Clamp(Stacks, 0, 255));

    if (FMythicRuneHudStateItem *Item = FindHudItem(SlotIndex)) {
        if (Item->State == State && Item->Stacks == StackCount && Item->ServerEndTimeSeconds == End) {
            return;
        }
        Item->State = State;
        Item->Stacks = StackCount;
        Item->ServerStartTimeSeconds = Now;
        Item->ServerEndTimeSeconds = End;
        HudStates.MarkItemDirty(*Item);
    }
    else {
        FMythicRuneHudStateItem &NewItem = HudStates.Items.AddDefaulted_GetRef();
        NewItem.SlotIndex = SlotIndex;
        NewItem.State = State;
        NewItem.Stacks = StackCount;
        NewItem.ServerStartTimeSeconds = Now;
        NewItem.ServerEndTimeSeconds = End;
        HudStates.MarkItemDirty(NewItem);
    }
    OnRuneHudStateChanged.Broadcast();
}

void UMythicRuneComponent::SetRuneHudStateForRune(const UMythicRuneDefinition *Rune, EMythicRuneHudState State, float DurationSeconds,
                                                  int32 Stacks) {
    const int32 SlotIndex = FindSlotOfRune(Rune);
    if (SlotIndex == INDEX_NONE) {
        return;
    }
    SetRuneHudState(SlotIndex, State, DurationSeconds, Stacks);
}

void UMythicRuneComponent::ClearSlotAbility(int32 SlotIndex) {
    if (!GrantedHandles.IsValidIndex(SlotIndex) || !GrantedHandles[SlotIndex].IsValid()) {
        return;
    }
    if (UAbilitySystemComponent *ASC = ResolveASC()) {
        ASC->ClearAbility(GrantedHandles[SlotIndex]);
    }
    GrantedHandles[SlotIndex] = FGameplayAbilitySpecHandle();
}

bool UMythicRuneComponent::IsSlotUnlocked(int32 SlotIndex) const {
    return SlotIndex >= 0 && SlotIndex < GetUnlockedSlots();
}

bool UMythicRuneComponent::IsRuneUnlocked(const UMythicRuneDefinition *Rune) const {
    if (!Rune) {
        return false;
    }
    if (!Rune->RequiredTag.IsValid()) {
        return true;
    }
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (const UMythicAchievementComponent *Achievements = PS->GetAchievementComponent()) {
            if (Achievements->IsAchievementUnlocked(Rune->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicUnlockComponent *Unlocks = PS->GetUnlockComponent()) {
            if (Unlocks->HasUnlockTag(Rune->RequiredTag)) {
                return true;
            }
        }
        if (const UMythicNarrativeStateComponent *Narrative = PS->GetNarrativeState()) {
            if (Narrative->HasStoryTag(Rune->RequiredTag)) {
                return true;
            }
        }
    }
    const UAbilitySystemComponent *ASC = ResolveASC();
    return ASC && ASC->HasMatchingGameplayTag(Rune->RequiredTag);
}

UMythicRuneDefinition *UMythicRuneComponent::GetRuneInSlot(int32 SlotIndex) const {
    return EquippedRunes.IsValidIndex(SlotIndex) ? EquippedRunes[SlotIndex].LoadSynchronous() : nullptr;
}

void UMythicRuneComponent::GrantSlot() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UnlockedSlots >= MaxSlots) {
        UE_LOG(Myth, Verbose, TEXT("Runes: slot grant ignored — all %d sockets are already open."), MaxSlots);
        return;
    }
    UnlockedSlots++;
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::RevokeSlot() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    // One socket always stays open: it is the floor RestoreRunes clamps a save to.
    if (UnlockedSlots <= 1) {
        UE_LOG(Myth, Verbose, TEXT("Runes: slot revoke ignored — the first socket never closes."));
        return;
    }

    const int32 Closing = UnlockedSlots - 1;
    if (GetRuneInSlot(Closing)) {
        ServerUnequipRune(Closing);
    }
    UnlockedSlots--;
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::RestoreRunes(const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &SavedRunes, int32 SavedUnlockedSlots) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    UnlockedSlots = FMath::Clamp(SavedUnlockedSlots, 1, FMath::Max(MaxSlots, 1));
    SizeSlots();

    for (int32 SlotIndex = 0; SlotIndex < EquippedRunes.Num(); SlotIndex++) {
        ClearSlotAbility(SlotIndex);
        EquippedRunes[SlotIndex].Reset();
    }
    bool bHudChanged = HudStates.Items.Num() > 0;
    if (bHudChanged) {
        HudStates.Items.Reset();
        HudStates.MarkArrayDirty();
    }

    UAbilitySystemComponent *ASC = ResolveASC();
    bool bRolled = false;
    for (int32 SlotIndex = 0; SlotIndex < SavedRunes.Num(); SlotIndex++) {
        const FSoftObjectPath SavedPath = SavedRunes[SlotIndex].ToSoftObjectPath();
        if (SavedPath.IsNull()) {
            continue;
        }
        if (!EquippedRunes.IsValidIndex(SlotIndex) || !IsSlotUnlocked(SlotIndex)) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — slot %d is no longer open (%d of %d)."),
                   *SavedPath.ToString(), SlotIndex, GetUnlockedSlots(), MaxSlots);
            continue;
        }

        bool bAlreadyWorn = false;
        for (int32 Other = 0; Other < SlotIndex && !bAlreadyWorn; Other++) {
            bAlreadyWorn = EquippedRunes[Other].ToSoftObjectPath() == SavedPath;
        }
        if (bAlreadyWorn) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — already worn in an earlier slot."), *SavedPath.ToString());
            continue;
        }

        UMythicRuneDefinition *Rune = SavedRunes[SlotIndex].LoadSynchronous();
        if (!Rune || !Rune->HasPayload()) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — it no longer resolves to a rune with an ability."),
                   *SavedPath.ToString());
            continue;
        }
        if (!IsRuneUnlocked(Rune)) {
            UE_LOG(Myth, Warning, TEXT("Runes: dropped saved rune '%s' — %s is no longer earned."), *GetNameSafe(Rune),
                   *Rune->RequiredTag.ToString());
            continue;
        }
        if (!ASC) {
            UE_LOG(Myth, Error, TEXT("Runes: restore failed — %s has no ability system."), *GetNameSafe(GetOwner()));
            break;
        }

        // The passive activates inside GiveAbility and reads its socket and its numbers back, so the socket must
        // already name the rune and the roll must already exist. A save older than rune rolls rolls here.
        bRolled |= EnsureRuneRolled(Rune);
        EquippedRunes[SlotIndex] = Rune;
        const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(Rune->Ability, 1, INDEX_NONE, Rune));
        if (!Handle.IsValid()) {
            UE_LOG(Myth, Error, TEXT("Runes: restore failed — could not grant '%s'."), *GetNameSafe(Rune));
            EquippedRunes[SlotIndex].Reset();
            bHudChanged |= ClearHudItem(SlotIndex);
            continue;
        }
        GrantedHandles[SlotIndex] = Handle;
    }

    if (bRolled) {
        OnRuneRollsChanged.Broadcast();
    }
    if (bHudChanged) {
        OnRuneHudStateChanged.Broadcast();
    }
    OnRunesChanged.Broadcast();
}

EMythicRuneRefusal UMythicRuneComponent::CanEquipRune(int32 SlotIndex, const UMythicRuneDefinition *Rune,
                                                      int32 &OutOtherSlot) const {
    OutOtherSlot = INDEX_NONE;

    // Range is judged against MaxSlots, not the array: a client asks before its first replication lands.
    if (SlotIndex < 0 || SlotIndex >= FMath::Max(MaxSlots, 1)) {
        return EMythicRuneRefusal::SlotOutOfRange;
    }
    if (!IsSlotUnlocked(SlotIndex)) {
        return EMythicRuneRefusal::SlotLocked;
    }
    if (!Rune) {
        return EMythicRuneRefusal::NoRune;
    }
    if (!Rune->HasPayload()) {
        return EMythicRuneRefusal::NoPayload;
    }
    if (!IsRuneUnlocked(Rune)) {
        return EMythicRuneRefusal::DeedMissing;
    }

    // Compare by path: a rune can be worn while its asset is unloaded, and a null Get() would read as a free slot.
    const FSoftObjectPath RunePath(Rune);
    if (EquippedRunes.IsValidIndex(SlotIndex) && EquippedRunes[SlotIndex].ToSoftObjectPath() == RunePath) {
        return EMythicRuneRefusal::AlreadyWornHere;
    }
    for (int32 Other = 0; Other < EquippedRunes.Num(); Other++) {
        if (Other != SlotIndex && EquippedRunes[Other].ToSoftObjectPath() == RunePath) {
            OutOtherSlot = Other;
            return EMythicRuneRefusal::WornElsewhere;
        }
    }
    if (!ResolveASC()) {
        return EMythicRuneRefusal::NoAbilitySystem;
    }
    return EMythicRuneRefusal::None;
}

FText UMythicRuneComponent::DescribeRefusal(EMythicRuneRefusal Reason, int32 OtherSlot) {
    switch (Reason) {
    case EMythicRuneRefusal::None:
        return FText::GetEmpty();
    case EMythicRuneRefusal::SlotOutOfRange:
    case EMythicRuneRefusal::SlotLocked:
        return NSLOCTEXT("Mythic", "RuneRefusedSealed", "That socket is sealed");
    case EMythicRuneRefusal::DeedMissing:
        return NSLOCTEXT("Mythic", "RuneRefusedDeed", "Not earned yet");
    case EMythicRuneRefusal::AlreadyWornHere:
        return NSLOCTEXT("Mythic", "RuneRefusedWornHere", "Already worn here");
    case EMythicRuneRefusal::WornElsewhere:
        // The refusal RPC carries no other slot; only a local CanEquipRune can name the socket.
        if (OtherSlot < 0) {
            return NSLOCTEXT("Mythic", "RuneRefusedWornElsewhereUnknown", "Worn in another socket");
        }
        return FText::Format(NSLOCTEXT("Mythic", "RuneRefusedWornElsewhere", "Worn in socket {0}"),
                             FText::AsNumber(OtherSlot + 1));
    default:
        return NSLOCTEXT("Mythic", "RuneRefusedGeneric", "Could not equip");
    }
}

void UMythicRuneComponent::Refuse(const TCHAR *Verb, int32 SlotIndex, EMythicRuneRefusal Reason,
                                  const UMythicRuneDefinition *Rune, int32 OtherSlot) {
    UE_LOG(Myth, Warning, TEXT("Runes: %s refused for slot %d — %s (%s, other slot %d, %s, %d of %d open)."), Verb,
           SlotIndex, *UEnum::GetValueAsString(Reason), *GetNameSafe(Rune), OtherSlot, *GetNameSafe(GetOwner()),
           GetUnlockedSlots(), MaxSlots);
    ClientRuneRequestRefused(SlotIndex, Reason);
}

void UMythicRuneComponent::ClientRuneRequestRefused_Implementation(int32 SlotIndex, EMythicRuneRefusal Reason) {
    OnRuneRefused.Broadcast(SlotIndex, Reason);
}

void UMythicRuneComponent::ServerEquipRune_Implementation(int32 SlotIndex, UMythicRuneDefinition *Rune) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    SizeSlots();

    int32 OtherSlot = INDEX_NONE;
    const EMythicRuneRefusal Reason = CanEquipRune(SlotIndex, Rune, OtherSlot);
    if (Reason != EMythicRuneRefusal::None) {
        Refuse(TEXT("equip"), SlotIndex, Reason, Rune, OtherSlot);
        return;
    }

    // Grant before clearing the old ability, so a failed grant leaves the socket exactly as the player saw it. The
    // socket names the new rune first because the passive activates inside GiveAbility and reads its socket back
    // through FindSlotOfRune; the first-socket roll lands first for the same reason, so the ability reads its
    // numbers as it activates.
    UAbilitySystemComponent *ASC = ResolveASC();
    const TSoftObjectPtr<UMythicRuneDefinition> Previous = EquippedRunes[SlotIndex];
    const bool bHudCleared = ClearHudItem(SlotIndex);
    const bool bRolled = EnsureRuneRolled(Rune);
    EquippedRunes[SlotIndex] = Rune;
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(Rune->Ability, 1, INDEX_NONE, Rune));
    if (!Handle.IsValid()) {
        EquippedRunes[SlotIndex] = Previous;
        if (bRolled) {
            RuneRolls.RemoveAt(RuneRolls.Num() - 1);
        }
        if (bHudCleared) {
            OnRuneHudStateChanged.Broadcast();
        }
        Refuse(TEXT("equip"), SlotIndex, EMythicRuneRefusal::GrantFailed, Rune, INDEX_NONE);
        return;
    }

    ClearSlotAbility(SlotIndex);
    GrantedHandles[SlotIndex] = Handle;
    if (bRolled) {
        OnRuneRollsChanged.Broadcast();
    }
    if (bHudCleared) {
        OnRuneHudStateChanged.Broadcast();
    }
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::ServerUnequipRune_Implementation(int32 SlotIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !EquippedRunes.IsValidIndex(SlotIndex)) {
        return;
    }
    if (EquippedRunes[SlotIndex].IsNull()) {
        return;
    }

    ClearSlotAbility(SlotIndex);
    EquippedRunes[SlotIndex].Reset();
    if (ClearHudItem(SlotIndex)) {
        OnRuneHudStateChanged.Broadcast();
    }
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::ServerMoveRune_Implementation(int32 FromSlot, int32 ToSlot) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    SizeSlots();

    if (!EquippedRunes.IsValidIndex(FromSlot)) {
        Refuse(TEXT("move"), ToSlot, EMythicRuneRefusal::SlotOutOfRange, nullptr, INDEX_NONE);
        return;
    }
    if (EquippedRunes[FromSlot].IsNull()) {
        Refuse(TEXT("move"), ToSlot, EMythicRuneRefusal::NoRune, nullptr, INDEX_NONE);
        return;
    }
    if (FromSlot == ToSlot) {
        Refuse(TEXT("move"), ToSlot, EMythicRuneRefusal::AlreadyWornHere, EquippedRunes[FromSlot].Get(), INDEX_NONE);
        return;
    }

    UMythicRuneDefinition *Rune = EquippedRunes[FromSlot].LoadSynchronous();
    int32 OtherSlot = INDEX_NONE;
    EMythicRuneRefusal Reason = CanEquipRune(ToSlot, Rune, OtherSlot);
    // The rune is worn in FromSlot by definition; that is the one socket the worn-elsewhere rule must forgive.
    if (Reason == EMythicRuneRefusal::WornElsewhere && OtherSlot == FromSlot) {
        Reason = EMythicRuneRefusal::None;
    }
    if (Reason != EMythicRuneRefusal::None) {
        Refuse(TEXT("move"), ToSlot, Reason, Rune, OtherSlot);
        return;
    }

    ClearSlotAbility(ToSlot);
    GrantedHandles[ToSlot] = GrantedHandles[FromSlot];
    EquippedRunes[ToSlot] = EquippedRunes[FromSlot];
    GrantedHandles[FromSlot] = FGameplayAbilitySpecHandle();
    EquippedRunes[FromSlot].Reset();

    bool bHudChanged = ClearHudItem(ToSlot);
    if (FMythicRuneHudStateItem *Carried = FindHudItem(FromSlot)) {
        Carried->SlotIndex = ToSlot;
        HudStates.MarkItemDirty(*Carried);
        bHudChanged = true;
    }
    if (bHudChanged) {
        OnRuneHudStateChanged.Broadcast();
    }
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::OnRep_Runes() {
    OnRunesChanged.Broadcast();
}

void UMythicRuneComponent::OnRep_RuneRolls() {
    OnRuneRollsChanged.Broadcast();
}
