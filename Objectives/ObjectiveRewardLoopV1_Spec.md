# Objective Reward Loop V1 Spec

## Scope

Objective Reward Loop V1 is a small, server-authoritative quest-completion slice:

- An NPC can offer one `UObjectiveDefinition`.
- The player receives clear offer feedback: assigned, already active, already completed, invalid, or out of range.
- Objectives progress from existing GAS gameplay events, including `GAS.Event.Item.Acquired` for supported item acquisition paths.
- Completion attempts/claims authored rewards exactly once. Failed or partial reward delivery is reported but not retried in V1.
- Save/load preserves active and completed objectives without reward replay.
- Existing floating notification/callout channels are acceptable for V1.

## Non-Goals

- Quest chains, prerequisites, branching, abandon, retry, or reset.
- Multi-objective quest assets.
- Persistent quest journal UI.
- Editor tooling or DataAsset validation beyond C++ tests.
- Quest-state-aware dialogue selection.
- Party-wide or shared objectives.
- Transactional rollback or retry after partial reward failure.

## Verified Current Code

- `UObjectiveDefinition` already models `TriggerEventTag`, `RequiredCount`, optional `RequiredPayloadTag`, optional magnitude counting, `FRewardsToGive Rewards`, `DisplayText`, and `CompletedText`.
- `UObjectiveTracker` stores `FObjectiveProgress { Definition, CurrentCount, bCompleted }` and replicates owner-only.
- Event progression already skips completed entries, matches trigger tag and optional payload tag, counts magnitude when configured, grants rewards on completion transition, and sends progress/completion notification.
- `ServerAddObjective` is idempotent but returns no result and sends no assignment, duplicate, or completed feedback.
- NPC quest offers exist as `AMythicNPCCharacter::QuestOffer`; dialogue and recruit server paths both call `OfferNpcQuestIfAny`.
- `NotifyItemAcquired` emits `GAS.Event.Item.Acquired` on the authoritative player ASC with quantity as magnitude and item type in `TargetTags`.
- Inventory grants and world pickups call `NotifyItemAcquired` only when `AmountAdded > 0`; inventory moves do not emit acquisition.
- Save/load already serializes objective asset path, count, and completed latch through character data.
- Objective rewards currently call `FRewardsToGive::Give(PC)`.

## Data Model

No new persisted `bRewardClaimed` field is required for V1.

`FObjectiveProgress::bCompleted` means the objective is complete and its completion reward attempt has already been consumed, even if reward delivery partially failed. `FSerializedObjectiveData::bCompleted` remains the replay-prevention latch. Old saves with no objectives load as an empty objective state.

Add transient/reporting types:

- `EObjectiveOfferResult`: `Assigned`, `AlreadyActive`, `AlreadyCompleted`, `NoOffer`, `OutOfRange`, `Invalid`.
- `EObjectiveNotifyCategory`: `Assignment`, `Duplicate`, `Progress`, `Completed`, `RewardResult`.
- Optional: `FObjectiveRewardGrantResult { bAttempted, bSucceeded, bSpawnedWorldDrop }`.

## Required C++ Changes

- Add `FObjectiveOfferResult UObjectiveTracker::ServerTryAddObjective(UObjectiveDefinition* Definition, FObjectiveProgress& OutProgress)`.
- `ServerTryAddObjective` is an internal authority-only method despite the `Server` prefix. It must not be exposed as a client-callable server RPC accepting `UObjectiveDefinition*`.
- Keep `ServerAddObjective` as a compatibility wrapper around `ServerTryAddObjective`.
- Add a client notification path for objective result states, such as `ClientNotifyObjectiveResult`.
- Update `AMythicPlayerController::OfferNpcQuestIfAny` to call `ServerTryAddObjective` and notify the owning client for valid offers.
- Completion should capture `const bool bRewardSucceeded = Rewards.Give(...)` and notify completion plus reward result.
- If detailed reward reporting is implemented, extend `FRewardsToGive::Give` with an optional out report instead of replacing the existing signature.
- Objective item rewards should attempt inventory delivery by passing the player controller as an `IInventoryProviderInterface` when possible. If inventory delivery overflows to a world item, surface `Reward Dropped Nearby`.

## Save And Load

- Save all tracked objectives as today.
- Restore completed objectives as completed and never re-grant rewards.
- Missing objective assets are skipped with a warning.
- No save version bump is required unless a new persisted field is added.

## Multiplayer And Authority

- Only the server assigns objectives, processes objective events, grants rewards, and restores objective state.
- Clients may request dialogue/recruit only; they never supply objective assets directly.
- NPC range remains server-validated through `IsActorInTradeRange`.
- Objective replication remains `COND_OwnerOnly`.
- Notifications go only to the owning client.
- Server-side NPC validation must reject null, invalid, wrong-world, destroyed, or out-of-range NPC references before assigning objectives.

## Player-Facing Result States

- Assignment: `Objective Assigned: {ObjectiveText}`.
- Duplicate active: `Objective Already Active: {ObjectiveText} {Current}/{Required}`.
- Duplicate completed: `Objective Already Completed: {ObjectiveText}`.
- No-offer is not shown during ordinary dialogue/recruit interactions; it is only a return state for code/tests to avoid noisy non-quest NPC interactions.
- Progress: `{ObjectiveText} {Current}/{Required}`.
- Completed: `Objective Complete: {CompletedTextOrDisplayText}`.
- Reward success: `Rewards Granted`.
- Reward world fallback: `Reward Dropped Nearby`.
- Reward failure/config error: `Reward Delivery Failed`.

## Test Plan

Add or extend automation tests in `Source/Mythic/Tests/LivingWorld/LivingWorldTests.cpp` unless a world-backed test is required.

- `Mythic.Objectives.ObjectiveTracker.OfferResultStates`.
- `Mythic.Objectives.ObjectiveTracker.ItemAcquiredPayloadMagnitude`.
- `Mythic.Objectives.ObjectiveTracker.CompletionGrantsRewardOnce`.
- `Mythic.Objectives.ObjectiveTracker.RestoreCompletedNoRewardReplay`.
- `Mythic.Objectives.ObjectiveNotification.MessageCategories`.
- Keep `Mythic.Objectives.ObjectiveTracker.Progress` as the arithmetic baseline.
