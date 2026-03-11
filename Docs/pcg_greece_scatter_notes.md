# Greece PCG Scatter Pass

This note is the current authoring target for [PCG_Greece_MediterraneanScatter](/Users/jacksonwells/Downloads/Many%20Names/Content/PCG/Regions/PCG_Greece_MediterraneanScatter.uasset).

## Goal

Break the current sparse, repetitive Greece terrain dressing into a more believable Mediterranean scatter layer without obscuring travel routes, ritual spaces, or interactable NPCs.

## Recommended Node Order

1. `Input`
2. `Surface Sampler`
3. `Transform Points`
4. `Density Filter`
5. `Static Mesh Spawner`

Duplicate the final `Static Mesh Spawner` for layered shrub/tree passes once the rock pass reads correctly.

## Node Settings

### Surface Sampler

- Source: landscape input
- Points per square meter: `0.1`

### Transform Points

- `Absolute Rotation`: enabled
- Rotation min: `(0, 0, -180)`
- Rotation max: `(0, 0, 180)`
- Scale min: `(0.5, 0.5, 0.5)`
- Scale max: `(2.0, 2.0, 2.0)`

This is the minimum randomness needed to stop the repeated static-mesh read.

### Density Filter

- Lower bound: `0.3`
- Upper bound: `1.0`

This keeps the biome patchy instead of uniformly painted.

### Static Mesh Spawner

Initial rock target:

- [Rock_001.uasset](/Users/jacksonwells/Downloads/Many%20Names/Content/Marketplace/Fab/PropsShared/Rock001/Rock_001.uasset)

Secondary layers after the rock pass:

- low shrubs
- hardy cliff plants
- olive-like accents only where they do not become anachronistically lush
- shrine debris or ritual stone fragments near sanctuary routes

## Exclusion Rules

Keep PCG out of:

- player starts
- primary roads and stairs
- plaza centers
- dialogue/cutscene anchor spaces
- shrine thresholds
- cliff-edge traversal lines

## Readability Bar

- main routes remain readable at a glance
- the sanctuary court must not look empty
- the mountain route must feel wind-cut, not overgrown
- the region should read as dry Mediterranean terrain, not fantasy forest
