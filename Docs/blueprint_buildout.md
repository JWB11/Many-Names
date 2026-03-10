# Blueprint Buildout Checklist

## Core blueprints
- `BP_ManyNamesGameMode`
  - Reads imported data tables.
  - Routes startup from the opening map into the region-selection flow.
- `BP_FirstPersonCharacter`
  - Movement, interaction trace, journal access, and mythic power input.
  - Keeps combat intentionally thin: one evasive move, one contextual attack, one power wheel.
- `BP_PlayerJournalWidget`
  - Shows active quests, mythic domains, rumor profile, and unlocked powers.
- `BP_DialogueWidget`
  - Binds to dialogue choice rows and sends selected results into `UManyNamesWorldStateSubsystem` and `UManyNamesMythSubsystem`.

## Map goals
- `L_OpeningCatastrophe`
  - Linear opening with traversal hazards, one witness encounter, and a three-way travel unlock.
- `L_EgyptHub`
  - Temple district hub with archive interior and necropolis spoke.
- `L_GreeceHub`
  - Sanctuary hub with mountain route and noble court spoke.
- `L_ItalicHub`
  - Hill settlement hub with ritual road and forge-law spoke.
- `L_Convergence`
  - Compact endgame map built around the buried descent apparatus.

## Encounter pattern
- Every major encounter should support:
  - one dialogue-forward resolution
  - one intimidation or omen-based resolution
  - one escalation into combat
  - one mythic-domain consequence

## Marketplace asset curation
- Egypt: sandstone temple kit, desert cliffs, necropolis props, solar motifs.
- Greece: marble sanctuary kit, scrub vegetation, mountain path pieces, bronze votive props.
- Italic west: timber-stone hill settlement kit, roads, boundary stones, forge props, civic markers.
- Unify all three with consistent post-process, decals, UI typography, and audio language.
