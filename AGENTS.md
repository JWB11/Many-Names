# AGENTS.md

## Project Snapshot

- Project: `Many Names`
- Genre: single-player first-person narrative RPG
- Engine: Unreal Engine 5.7 on desktop (`Mac` and `Windows` target intent)
- Current state: playable prototype with five staged maps and a working C++/Blueprint foundation
- Canon premise: the player is an amnesiac crash-landed astronaut whose survival tech is interpreted as miracle and divinity in the ancient Mediterranean

## Canon And Tone

- Keep the project historically grounded across Egypt, Archaic Greece, and the early Italic west.
- Myth must emerge from interpretation, ritual, rumor, and politics, not from overt fantasy exposition.
- Do not reveal the sci-fi truth too early.
- Technology should read as uncanny miracle, not modern weaponry.
- Maintain serious, sacred, tragic-adventure tone.
- Avoid generic fantasy, superhero framing, or pulp action drift.

## Implemented Runtime Truth

- Core gameplay loop is active:
  - first-person movement and interaction
  - quest dialogue resolution
  - world-state outputs and save/load
  - mythic domain rewards
  - region travel
  - region completion and convergence unlock logic
- Canonical runtime systems:
  - `UManyNamesGameInstance`
  - `UManyNamesWorldStateSubsystem`
  - `UManyNamesMythSubsystem`
  - `UManyNamesQuestSubsystem`
  - `UManyNamesContentSubsystem`
- The authored data in `Data/` is the source of truth for region rows, quest rows, dialogue rows, quest steps, choice consequences, and ending gates.
- Do not redesign save schema, quest ids, world-state outputs, or dialogue schema casually.

## Current Playable Maps

- `/Game/Maps/L_OpeningCatastrophe`
- `/Game/Maps/L_EgyptHub`
- `/Game/Maps/L_GreeceHub`
- `/Game/Maps/L_ItalicHub`
- `/Game/Maps/L_Convergence`

## Current Quest Flow

- Opening:
  - `opening_main_01`
  - `opening_side_01`
- Egypt:
  - `egypt_main_01`
  - `egypt_side_01`
- Greece:
  - `greece_main_01`
  - `greece_side_01`
- Italic West:
  - `italic_main_01`
  - `italic_side_01`
- Convergence:
  - `convergence_main_01`

## Mythic Domains

The current implemented domain set is:

- `Light`
- `Storm`
- `Healing`
- `Craft`
- `Death`
- `Judgment`
- `Order`
- `Deception`

Use these domains instead of adding alignment systems or expanding into broad speculative lists unless the project explicitly needs it.

## Art Direction

- Use *Indiana Jones and the Dial of Destiny* only as a broad mood reference.
- Allowed reference traits:
  - warm adventure realism
  - archaeological density
  - dust, soot, age, and travel wear
  - practical materials
  - cinematic silhouettes and readable scale
- Not allowed:
  - copying film-specific props
  - copying set layouts
  - copying costumes
  - copying franchise iconography
- Region art language:
  - Egypt: layered sacred masonry, solar authority, archive density, necropolis age
  - Greece: exposed sky, cliff drama, banners, ritual stone, storm spectacle
  - Italic West: timber-stone pragmatism, civic markers, roads, boundary law, forge order
  - Convergence: sparse buried sci-fi embedded in archaeology, severe and restrained

## Asset Strategy

- Imported Fab content lives under `Content/Marketplace/Fab`.
- Project-owned curated layers should remain under project folders such as:
  - `Content/Characters`
  - `Content/Materials`
  - `Content/Blueprints`
  - `Content/Maps`
- Current imported Fab coverage is mainly environment and prop content, not real civilian/human character coverage.
- Current imported Fab buckets in use:
  - `EnvironmentEgypt`
  - `EnvironmentGreece`
  - `EnvironmentItalic`
  - `EnvironmentConvergence`
  - `PropsShared`
  - `ArtifactsHero`
- Current named and ambient NPC reality:
  - runtime now supports profile-driven NPC visuals and idle animation through `FManyNamesNpcVisualProfile`
  - staged maps currently use mannequin-based stand-ins plus available posed/imported asset support
  - MetaHumans or realistic Fab human packs are still desired for named NPC replacement, but they are not yet imported into this project
- Do not claim human replacement is complete unless real human assets are actually imported and assigned.

## Visual And World Build Tooling

- Dynamic lighting is the project default for prototype maps.
- Lighting warning fix already applied:
  - `Force No Precomputed Lighting`
  - movable sun and sky light
- Current world-build toolchain:
  - `scripts/unreal_import_fab_assets.py`
  - `scripts/unreal_inspect_assets.py`
  - `Source/ManyNames/Private/Editor/ManyNamesWorldBuildCommandlet.cpp`
- The world-build commandlet now:
  - loads imported Fab environment assets when available
  - falls back to project primitives when necessary
  - stages all five maps
  - applies dynamic sky/fog/light setup
  - places quest anchors, travel gates, ambient NPCs, and region set dressing
- If imported asset names change, fix the commandlet path assumptions rather than hand-correcting maps silently.

## Current Session Outcomes

The following work is already implemented and should be treated as current project truth:

- authored data pack expanded and validated
- supplemental JSON content loader added
- Blueprint/UI integration docs written
- asset sourcing bible written
- dynamic lighting setup applied to prototype maps
- initial material pass added
- Fab asset import pipeline created and used
- Fab asset manifest generated:
  - `Data/fab_asset_manifest.json`
- Fab asset inventory generated:
  - `Docs/fab_asset_inventory.md`
- opening and Egypt expanded beyond the original greybox
- Greece, Italic West, and Convergence maps generated and staged
- mannequin path dependency bug fixed in prior work
- role-based NPC visual profile support added
- idle and death animation usage added for staged mannequin stand-ins
- automation coverage expanded for staged region map references
- world build commandlet now completes with `0 errors, 0 warnings`

## Controls And Flow

- Current prototype controls:
  - `WASD` move
  - mouse look
  - `Space` jump
  - `E` interact
  - `J` journal
  - `Q` Focus Shift
  - `R` Insight Pulse
  - `1/2/3/4` dialogue or region selection
- Current destination flow:
  - Opening unlocks Egypt, Greece, and Italic West
  - Completing Egypt, Greece, and Italic West unlocks Convergence

## Validation And Regression Checks

- Content validation:
  - `python3 scripts/validate_content.py`
- C++ automation:
  - `ManyNames.Core.DefaultWorldState`
  - `ManyNames.Core.PrimaryContentLoad`
- Headless world rebuild:
  - run `ManyNamesWorldBuild` commandlet
- Map check target:
  - no lighting rebuild warnings
  - `0 errors, 0 warnings` during commandlet map processing

## Implementation Constraints

- Preserve the current data-driven architecture.
- Prefer extending authored data and world-build tooling over hardcoding one-off logic.
- Use `apply_patch` for file edits.
- Prefer dynamic lighting and movable lights unless there is a specific reason to reintroduce baked lighting.
- Keep Blueprint-first workflow intact; only add C++ where Blueprint would become brittle.
- Keep marketplace/Fab content as source content and build curated project usage on top.

## Known Remaining Gaps

- Named NPCs still need replacement with actual human assets when suitable MetaHumans or realistic Fab character packs are imported.
- Ambient population is still simple staged idle presence, not full AI simulation.
- Egypt is ahead of the other regions in architectural specificity; Greece, Italic West, and Convergence are staged and playable but still need deeper art passes.
- Some structures remain hybrid Fab-plus-greybox because the current Fab library is heavier on isolated assets than complete modular regional kits.
- Blueprint wrappers exist, but deeper UI polish and presentation are still pending.

## Guidance For Future Agents

- Start by checking authored data, current maps, and the world-build commandlet before making broad structural changes.
- Treat the current commandlet-generated maps as disposable output derived from code and content, not precious hand-authored source.
- If you add or import new Fab packs, update:
  - `scripts/unreal_import_fab_assets.py`
  - `Data/fab_asset_manifest.json`
  - `Docs/fab_asset_inventory.md`
  - the world-build commandlet asset paths
- If real human assets arrive, update NPC role assignment through `FManyNamesNpcVisualProfile` rather than introducing another parallel character-placement system.
- Any change that broadens tone, reveals the sci-fi truth too bluntly, or turns regions into one blended generic antiquity should be treated as a design bug and corrected.
