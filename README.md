# Many Names

`Many Names` is a UE5 first-person narrative RPG prototype set in the late 8th century BCE Mediterranean. This repository now includes the project skeleton, core gameplay state code, editor bootstrap scripts, and a first playable opening-to-Egypt bring-up.

## Included
- UE5 project file: [ManyNames.uproject](ManyNames.uproject)
- Runtime module with:
  - world state and save/load flow
  - mythic domain tracking
  - quest state scaffolding
  - authored primary content loading from source JSON for regions, quests, and dialogue
  - supplemental content loading for quest steps, choice consequences, and ending gates
  - a shared interaction interface
- Bootstrapped Unreal assets:
  - `/Game/Data/DT_Regions`
  - `/Game/Data/DT_Quests`
  - `/Game/Data/DT_DialogueChoices`
  - `/Game/Blueprints/Core/BP_ManyNamesGameMode`
  - `/Game/Blueprints/Core/BP_FirstPersonCharacter`
  - `/Game/Blueprints/Core/BP_DialogueController`
  - `/Game/Blueprints/UI/BP_PlayerJournalWidget`
  - `/Game/Blueprints/Interaction/BP_Interactable`
  - `/Game/Maps/L_OpeningCatastrophe`
  - `/Game/Maps/L_EgyptHub`
- Import-ready prototype data:
  - [regions.json](Data/regions.json)
  - [quests.json](Data/quests.json)
  - [dialogue_choices.json](Data/dialogue_choices.json)
  - [quest_steps.json](Data/quest_steps.json)
  - [choice_consequences.json](Data/choice_consequences.json)
  - [ending_gates.json](Data/ending_gates.json)
- Project config for desktop targeting and startup wiring
- Blueprint and art build specs:
  - [blueprint_ui_spec.md](Docs/blueprint_ui_spec.md)
  - [asset_sourcing_bible.md](Docs/asset_sourcing_bible.md)

## Bring-up scripts
- `python3 scripts/validate_content.py`
  - validates authored quest, choice, and ending data before editor import/build steps
- `scripts/unreal_bootstrap_bringup.py`
  - creates or refreshes the core DataTables, Blueprint wrappers, and starter maps through `UnrealEditor-Cmd`
- `scripts/unreal_verify_datatables.py`
  - checks imported DataTable columns to confirm tag-heavy fields resolved correctly

## Next steps in Unreal Editor
1. Open [ManyNames.uproject](ManyNames.uproject) in Unreal Engine 5.x.
2. Confirm the project opens on `L_OpeningCatastrophe` and uses `BP_ManyNamesGameMode` as the default game mode.
3. Press Play and verify the first loop:
   - interact with the miracle anchor
   - resolve the witness dialogue
   - unlock region selection
   - travel to Egypt
   - resolve the archive dialogue in `L_EgyptHub`
4. Replace greybox geometry with marketplace kits guided by [asset_sourcing_bible.md](Docs/asset_sourcing_bible.md).
5. Build out the next Blueprint/UI layer from [blueprint_ui_spec.md](Docs/blueprint_ui_spec.md), starting with a real journal widget and dialogue widget.
6. Stage Greece, Italic west, and convergence maps once the opening-plus-Egypt loop is stable.

## Suggested folder conventions
- `Content/Blueprints`
- `Content/UI`
- `Content/Maps`
- `Content/Data`
- `Content/Marketplace`
- `Content/Audio`

## Content validation
- Run `python3 scripts/validate_content.py` to verify quest ids, choice ids, region coverage, and combat/non-combat path coverage before importing changes into Unreal.

## Core gameplay model
- The player can visit Egypt, Greece, or the Italic west in any order after the opening.
- Mythic Domains replace classes and drive story interpretation.
- Combat is conditional and rises or falls from player choice.
- Endings are derived from region order, companion outcomes, domain profile, and major decisions.
