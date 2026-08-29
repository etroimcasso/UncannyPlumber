# Features

Status registry — one row per feature. `features/CHANGELOG.md` records every status transition
chronologically; this file holds current state.

**Legend:** ⬜ pending · 🟡 in progress · ✅ delivered · 🟫 dropped · ❌ rejected

---

## Infrastructure

| Feature | Status | Doc |
|---|---|---|
| Repository scaffolding and ignore rules | ✅ | — |
| Build system | ✅ | — (Release by default, dead-stripped; the build doc lands with CI) |
| Test harness | ✅ | — |
| Retro++ engine adoption | ✅ | — (submodule at `594774c`; consumer only) |
| Logging | ✅ | — (spdlog used directly; no wrapper) |
| Asset acquisition | ⬜ | [`features/asset-acquisition.md`](features/asset-acquisition.md) |
| ROM extraction (graphics, sound driver, levels) | ⬜ | [`features/asset-acquisition.md`](features/asset-acquisition.md) |
| Continuous integration | ⬜ | `features/ci.md` |
| Distributable build | ⬜ | `features/distributable-build.md` |

## Data

Constant tables derived from the ROM (compiled in) and content extracted from it (never committed).

| Feature | Status | Doc |
|---|---|---|
| Core enums (enemy kinds, game states, sound IDs) | ⬜ | — |
| Character map | ⬜ | — |
| Tile graphics (four worlds, common, menu) | ⬜ | — |
| Level data (twelve levels, column format) | ⬜ | — |
| Enemy placement tables | ⬜ | — |
| Music and sound-effect data | ⬜ | — |
| Physics and timing constants | ⬜ | — |
| Scoring tables | ⬜ | — |

Rows are added as the data audit enumerates the corpus.

## State

| Feature | Status | Doc |
|---|---|---|
| Global game state | ⬜ | — |
| Player state | ⬜ | — |
| Object / enemy slots | ⬜ | — |
| Level scroll and column streaming state | ⬜ | — |
| Audio state | ⬜ | — |

## Systems

| Feature | Status | Doc |
|---|---|---|
| Randomness (divider reads on the VM) | ⬜ | — |
| Input | ⬜ | — |
| Game-state dispatcher | ⬜ | — |
| Player movement and physics | ⬜ | — |
| Collision (tiles, blocks, coins) | ⬜ | — |
| Enemy behaviors | ⬜ | — |
| Powerups (mushroom, flower / superball, star) | ⬜ | — |
| Level streaming (column draw) | ⬜ | — |
| Bosses (per world) | ⬜ | — |
| Bonus game | ⬜ | — |
| Title / continue / game over screens | ⬜ | — |
| Vehicles (Marine Pop, Sky Pop) | ⬜ | — |
| Chiptune audio backend | ⬜ | — |
| Anti-channel-stealing option | ⬜ | — |
| Top-score persistence | ⬜ | — |
| Boot path | ⬜ | — |

## Rendering

| Feature | Status | Doc |
|---|---|---|
| Background rendering and scroll | ⬜ | — |
| Status bar (mid-frame display change) | ⬜ | — |
| Sprite renderer | ⬜ | — |
| Settings screen | ⬜ | — |
| Colour palettes | ⬜ | — |
| Integer-scale / free-aspect output | ⬜ | — |
| DMG display shader | ⬜ | — |

## Entry and integration

| Feature | Status | Doc |
|---|---|---|
| Entry point and boot host | ⬜ | — |
| First-start ROM selection | ⬜ | [`features/asset-acquisition.md`](features/asset-acquisition.md) |
