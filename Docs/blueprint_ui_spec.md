# Blueprint And UI Integration Spec

## Goal
This spec locks the first Unreal Editor buildout against the current C++ runtime and authored data. The Blueprint layer should not invent state ids, quest flow, or UI behavior beyond what is defined here and in `Data/`.

## Runtime contract
- Data sources:
  - primary JSON loaded at runtime through `UManyNamesContentSubsystem`:
    - `Data/regions.json`
    - `Data/quests.json`
    - `Data/dialogue_choices.json`
  - supplemental JSON loaded at runtime through `UManyNamesContentSubsystem`:
    - `Data/quest_steps.json`
    - `Data/choice_consequences.json`
    - `Data/ending_gates.json`
  - imported Data Tables remain editor mirrors for inspection and Blueprint convenience:
    - `DT_Regions`
    - `DT_Quests`
    - `DT_DialogueChoices`
- Core state owners:
  - `UManyNamesGameInstance` owns the canonical `FManyNamesWorldState`
  - `UManyNamesWorldStateSubsystem` writes region visits, endings, choices, powers, and combat reputation
  - `UManyNamesMythSubsystem` writes domain scores and regional interpretations
  - `UManyNamesQuestSubsystem` tracks runtime quest states per loaded world
  - `UManyNamesContentSubsystem` loads and caches both primary and supplemental authored JSON
- Blueprint rule:
  - Blueprints may read from Data Tables and call subsystem functions.
  - Blueprints must not invent alternative save-state containers.
  - Blueprints should read supplemental quest-step, consequence, and ending-gate data through `UManyNamesContentSubsystem`, not by hand-maintaining duplicate editor data.

## Blueprint classes
### `BP_ManyNamesGameMode`
- Responsibilities:
  - Reload authored content on BeginPlay through `UManyNamesContentSubsystem`.
  - Treat imported Data Tables as editor mirrors, not the canonical runtime source.
  - Start the opening catastrophe on a fresh save.
  - If `State.Region.Opening.Complete` is already true, route to the last valid hub or region-select overlay.
  - Expose a `StartRegionTravel(RegionId)` event that checks region unlock state before changing maps.
- Required functions/events:
  - `InitializePrototypeRun()`
  - `HandleQuestCompleted(QuestId, WorldStateOutputId)`
  - `HandleRegionResolved(RegionId)`
  - `OpenRegionSelect()`
  - `TryEnterConvergence()`

### `BP_FirstPersonCharacter`
- Responsibilities:
  - Own first-person movement, look, jump, interaction trace, journal open/close, and power inputs.
  - Dispatch interaction hits against `IManyNamesInteractable`.
  - Trigger contextual powers only if the power is unlocked in `WorldState.UnlockedPowers`.
  - Keep combat narrow: one light attack, one evade, one contextual mythic power activation.
- Required functions/events:
  - `TraceForInteractable()`
  - `AttemptInteract()`
  - `CanUsePower(PowerId)`
  - `UseFocusShift()`
  - `UseInsightPulse()`
  - `SetThreatState(IsThreatened)`

### `BP_DialogueController`
- Responsibilities:
  - Accept a `QuestId` and a prompt context, then show all matching choice rows.
  - Filter out choices that fail `RequiredDomains`.
  - On player selection, look up the matching consequence record through `UManyNamesContentSubsystem`.
  - Apply world-state outputs, domain deltas, rumor deltas, companion affinity deltas, and ending eligibility in a fixed order.
- Apply order:
  - `ApplyChoice(ChoiceId, SelectedOptionId)` on `UManyNamesWorldStateSubsystem`
  - domain score updates on `UManyNamesMythSubsystem`
  - combat reputation update on `UManyNamesWorldStateSubsystem`
  - companion affinity and alliance changes in Blueprint against the copied world state, then `SetWorldState`
  - mark quest state progression in `UManyNamesQuestSubsystem`
- Required functions/events:
  - `OpenDialogue(QuestId, PromptText)`
  - `GetAvailableChoices(QuestId)`
  - `ResolveChoice(ChoiceId)`
  - `ApplyConsequenceRecord(ChoiceId)`
  - `CloseDialogue()`

### `BP_PlayerJournalWidget`
- Responsibilities:
  - Display active quests grouped by region.
  - Show current Mythic Domains and which ones are public.
  - Show rumor profile as three meters: public miracle, concealment, combat reputation.
  - Show unlocked powers and convergence ending eligibility when available.
- Required tabs:
  - `Quests`
  - `Domains`
  - `Rumors`
  - `Powers`
  - `Endgame` unlocked only after all three main regional quests are complete
- Required functions/events:
  - `RefreshFromWorldState(WorldState)`
  - `RefreshQuestList()`
  - `RefreshDomains()`
  - `RefreshRumors()`
  - `RefreshEndings()`

### `BP_DialogueWidget`
- Responsibilities:
  - Present NPC line, player options, requirements, and projected tone.
  - Show locked options as disabled with a domain requirement label.
  - Return a single selected `ChoiceId` to `BP_DialogueController`.
- Required visual states:
  - available option
  - locked option
  - high-risk option if `CombatDelta > 0`
  - concealment option if it increases rumor concealment

### `BP_Interactable_*`
- Responsibilities:
  - Implement `ManyNamesInteractable`.
  - Use one of five action types only: inspect, talk, activate, collect clue, trigger omen.
  - Route interaction to quest/dialogue logic instead of carrying local state whenever possible.

## Player-facing flow
### Opening flow
- Spawn in `L_OpeningCatastrophe`.
- Complete `opening_main_01` step sequence.
- Trigger `opening_side_01` once the witness is reachable.
- On `Story.Prologue.Complete`, open a three-region selection overlay.
- Persist the selected first destination in world state and load the chosen region map.

### Region hub flow
- On hub load:
  - register region visit
  - set the main quest to active if prerequisites are met
  - unlock the side quest once the player reaches the hub center
- Each region hub must expose:
  - one main questline anchor NPC or relic
  - one side questline anchor
  - one optional combat encounter that can be bypassed by prior choices

### Dialogue flow
- Player interacts with an NPC or relic.
- `BP_DialogueController` opens a prompt for the current quest step.
- UI lists all rows matching the current `QuestId`.
- Selection resolves through `choice_consequences.json`.
- Journal refreshes immediately after the consequence is applied.

### Ending flow
- `convergence_main_01` step 03 is only playable after all three regional main quests are complete.
- `BP_DialogueController` should filter ending options against `ending_gates.json`.
- The widget should show unavailable endings as locked with the missing gate label.
- Final selection writes the ending output id and triggers the corresponding end-state sequence.

## Data flow
- Region/quest bootstrap:
  - `BP_ManyNamesGameMode` reads region and quest rows from `UManyNamesContentSubsystem`.
  - it activates the correct quest states in `UManyNamesQuestSubsystem`.
- Choice resolution:
  - `BP_DialogueController` reads `DT_DialogueChoices`
  - it cross-references `UManyNamesContentSubsystem`
  - it writes to `UManyNamesWorldStateSubsystem` and `UManyNamesMythSubsystem`
  - it forces a journal refresh
- Journal updates:
  - `BP_PlayerJournalWidget` binds to `UManyNamesGameInstance.OnWorldStateChanged`
  - it pulls a fresh copy of world state every time the delegate fires
- Ending evaluation:
  - `BP_DialogueController` evaluates ending rules through `UManyNamesContentSubsystem`
  - successful gates call `SetEligibleEnding`

## Minimal Blueprint utility types
- `UManyNamesContentSubsystem`
  - use `GetQuestStepsForQuest`
  - use `GetChoiceConsequence`
  - use `GetEndingGate`
  - use the conversion helpers for ending and companion names when applying consequences

## Do not add in first editor pass
- voiced cinematics
- inventory economy
- weapon trees
- AI companions
- large systemic stealth
- procedural quest generation
