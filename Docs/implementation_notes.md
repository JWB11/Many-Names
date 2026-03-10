# Many Names Prototype Notes

## Current state
- This repository now contains a hand-authored Unreal Engine 5 project skeleton.
- The focus is the durable gameplay/state layer, not binary `.uasset` content that requires the editor.
- `Data/*.json` provides the most reliable import path for Unreal Data Tables with arrays and gameplay tags.
- `Data/*.csv` is included as a spreadsheet-friendly reference version.
- Supplemental build-ready prototype content now lives in:
  - `Data/quest_steps.json`
  - `Data/choice_consequences.json`
  - `Data/ending_gates.json`
- The runtime bridge for those files is `UManyNamesContentSubsystem`, which loads and caches them from project-relative paths defined in `UManyNamesDeveloperSettings`.
- Editor-facing implementation guidance now lives in:
  - `Docs/blueprint_ui_spec.md`
  - `Docs/asset_sourcing_bible.md`

## Intended Unreal setup
- Create Blueprint subclasses for:
  - `BP_ManyNamesGameMode`
  - `BP_FirstPersonCharacter`
  - `BP_DialogueController`
  - `BP_Interactable_*` actors using `ManyNamesInteractable`
- Import the CSV files into Data Tables using:
  - `FManyNamesRegionRow`
  - `FManyNamesQuestRow`
  - `FManyNamesDialogueChoiceRow`
- Create maps referenced in config:
  - `/Game/Maps/L_OpeningCatastrophe`
  - `/Game/Maps/L_EgyptHub`
  - `/Game/Maps/L_GreeceHub`
  - `/Game/Maps/L_ItalicHub`
  - `/Game/Maps/L_Convergence`

## Prototype content targets
- Opening catastrophe proves traversal, first miracle, witness choice, and region selection.
- Each region needs one hub, one side activity, one major deity confrontation, and a combat escalation path.
- Final convergence resolves companion states and grants ending eligibility based on prior play.
- The supplemental JSON files are the source of truth for quest step order, choice outcomes, companion affinity shifts, rumor deltas, and ending gates until dedicated Unreal assets replace them.

## Known limits
- No generated `.uasset` files are included because Unreal Editor is not installed in this workspace.
- Input uses classic mappings in config; Enhanced Input assets still need to be authored in-editor.
- Quest and dialogue subsystems are intentionally lightweight and expect Blueprint/UI binding on top.
