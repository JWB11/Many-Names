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
  - branch-based dominant-antagonist escalation for `OracleAI`, `SkyRuler`, or `BronzeLawgiver`
- Canonical runtime systems:
  - `UManyNamesGameInstance`
  - `UManyNamesWorldStateSubsystem`
  - `UManyNamesMythSubsystem`
  - `UManyNamesQuestSubsystem`
  - `UManyNamesContentSubsystem`
- The authored data in `Data/` is the source of truth for region rows, quest rows, dialogue rows, dialogue scenes, quest steps, choice consequences, ending gates, cinematic scene rows, audio profiles, cast profiles, ambient crowd profiles, and external license records.
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
  - `egypt_side_02`
- Greece:
  - `greece_main_01`
  - `greece_side_01`
  - `greece_side_02`
- Italic West:
  - `italic_main_01`
  - `italic_side_01`
  - `italic_side_02`
- Convergence:
  - `convergence_main_01`
  - `convergence_side_02`

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
  - runtime supports profile-driven NPC visuals, idle animation, cloth tier metadata, voice ids, and foot-IK profile ids through `FManyNamesNpcVisualProfile`
  - named story roles are authored as project MetaHumans under `Content/Characters/MetaHumans`
  - runtime MetaHuman assemblies are tracked through `Data/metahuman_manifest.json`
  - ambient population is still lighter profile-driven crowd staging rather than full AI simulation
- Do not reintroduce Manny/Quinn or mannequin fallback for named story roles.

## Visual And World Build Tooling

- Dynamic lighting is the project default for prototype maps.
- Nanite is enabled at the project level for environment-scale static meshes where supported.
- PCG, Landmass, Water, Geometry Scripting, Niagara Fluids, Control Rig, IKRig, Sequencer Scripting, Movie Render Pipeline, World Partition HLOD Utilities, Chaos Cloth Editor, and Chaos Cloth Asset Editor are enabled for terrain/world expansion work.
- Lighting warning fix already applied:
  - `Force No Precomputed Lighting`
  - movable sun and sky light
- Current world-build toolchain:
  - `scripts/generate_alpha_content.py`
  - `scripts/generate_alpha_audio.py`
  - `scripts/unreal_import_fab_assets.py`
  - `scripts/unreal_import_generated_audio.py`
  - `scripts/unreal_inspect_assets.py`
  - `scripts/unreal_generate_cinematics.py`
  - `scripts/unreal_create_metahuman_cast.py`
  - `scripts/unreal_complete_metahuman_cast.py`
  - `Source/ManyNames/Private/Editor/ManyNamesWorldBuildCommandlet.cpp`
- The world-build commandlet now:
  - loads imported Fab environment assets when available
  - falls back to project primitives when necessary
  - stages all five maps
  - scales terrain to alpha-target regional footprints
  - applies dynamic sky/fog/light setup
  - creates project-owned PCG graph assets under `Content/PCG`
  - spawns route spline actors from terrain profiles
  - creates real `ALandscape` terrain foundations for all five maps
  - enables Nanite on eligible structural meshes referenced by terrain profiles
  - projects and validates `PlayerStart` against blocking ground
  - places quest anchors, travel gates, ambient NPCs, crowd clusters, cutscene anchors, and region set dressing
- Terrain is now generated from real commandlet-created Landscape actors plus hard-route overlays and structural staging.
- PCG is currently implemented as generated graph assets plus placed PCG volumes/profile scaffolding; deeper node-authored scatter graphs are still a later pass.
- If imported asset names change, fix the commandlet path assumptions rather than hand-correcting maps silently.

## Current Session Outcomes

The following work is already implemented and should be treated as current project truth:

- authored data pack expanded and validated
- supplemental JSON content loader added
- cinematic scene, audio profile, and external license loading added to `UManyNamesContentSubsystem`
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
- runtime spawn recovery added to the first-person character
- opening spawn bug fixed by moving and validating `PlayerStart` over blocking terrain
- thin primary ground planes replaced with thick collision-bearing terrain foundations in generated maps
- terrain profile scaffolding added for future Landscape/PCG/Landmass rollout
- all five generated maps now include real Landscape-backed terrain, route spline anchors, and project-owned PCG graph assets
- region bootstrap and journal flow now derive from authored quest rows instead of the original fixed quest list
- alpha data generator now expands the project to 13 quests, 38 quest steps, 22 cinematic scenes, 16 audio profiles, and roughly 30 cast records
- authored audio source generation and Unreal audio import scripts now exist for temporary motifs, ambience, stingers, and English placeholder voices
- placeholder Sequencer generation now exists for cutscene assets referenced from `Data/cinematic_scenes.json`
- additional named MetaHuman story roles were authored for the alpha cast expansion
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
- Spawn safety target:
  - every map must contain exactly one `PlayerStart`
  - that `PlayerStart` must validate against blocking ground during world build
- Content validation now also checks:
  - cinematic scene bindings to quest ids, dialogue scenes, cast ids, audio ids, and sequence assets
  - generated audio source files and imported Unreal sound assets
  - external asset/tool license coverage
  - MetaHuman manifest coverage for named cast ids

## Implementation Constraints

- Preserve the current data-driven architecture.
- Prefer extending authored data and world-build tooling over hardcoding one-off logic.
- Use `apply_patch` for file edits.
- Prefer dynamic lighting and movable lights unless there is a specific reason to reintroduce baked lighting.
- Keep Blueprint-first workflow intact; only add C++ where Blueprint would become brittle.
- Keep marketplace/Fab content as source content and build curated project usage on top.

## Known Remaining Gaps

- The expanded alpha cast still requires runtime MetaHuman completion and restaging whenever new named roles are added beyond the currently assembled manifest.
- Ambient population is still staged crowd presence, not full AI simulation.
- Egypt is ahead of the other regions in architectural specificity; Greece, Italic West, and Convergence are staged and playable but still need deeper art passes.
- Some structures remain hybrid Fab-plus-greybox because the current Fab library is heavier on isolated assets than complete modular regional kits.
- Landscape is now live in the generated maps, but Landmass shaping, authored PCG node graphs, and Geometry Script-generated connective meshes still need a deeper follow-up pass.
- Blueprint wrappers exist, but deeper UI polish, cutscene blocking polish, and audio mix polish are still pending.

## Guidance For Future Agents

- Start by checking authored data, current maps, and the world-build commandlet before making broad structural changes.
- Treat the current commandlet-generated maps as disposable output derived from code and content, not precious hand-authored source.
- Treat `Data/cinematic_scenes.json`, `Data/audio_profiles.json`, and `Data/external_asset_licenses.json` as first-class authored data alongside quest/dialogue rows.
- If you add or import new Fab packs, update:
  - `scripts/unreal_import_fab_assets.py`
  - `Data/fab_asset_manifest.json`
  - `Docs/fab_asset_inventory.md`
  - the world-build commandlet asset paths
- If new named human roles are added, create/update their MetaHumanCharacter assets and then rerun the automated completion pipeline so `Data/metahuman_manifest.json` stays authoritative.
- Update NPC role assignment through `FManyNamesNpcVisualProfile` rather than introducing another parallel character-placement system.
- Any change that broadens tone, reveals the sci-fi truth too bluntly, or turns regions into one blended generic antiquity should be treated as a design bug and corrected.
