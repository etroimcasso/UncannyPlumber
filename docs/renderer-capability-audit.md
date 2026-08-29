# Renderer Capability Audit — Super Mario Land Observables → Retro++ Surface

**Date:** 2026-08-28
**Status:** Complete
**Inputs:** a full survey of PPU usage in the Super Mario Land disassembly
(`kaspermeerts/supermarioland` @ `618d00e` — every video-register, VRAM and OAM access in
`bank0.asm`, `bank1.asm`, `bank2.asm`, `bank3.asm` and the two undisassembled frame handlers at
`$0627` and `$2376`, read from the ROM; `music.asm` touches no video state); Retro++ public headers
and implementation at engine `594774c`.
**Feeds:** the rendering design, the graphics extractor's output layout, and engine work-item
sequencing — of which there is none, see the verdict.

This document maps every renderer-visible behavior in Super Mario Land onto the Retro++ engine's
drawing vocabulary and issues one verdict per behavior region:

- **Covered** — the engine expresses the observable directly.
- **Covered with pattern** — the engine expresses it through a documented composition of existing
  surfaces (the pattern is named).
- **Engine work item** — a genuine capability gap; named in engine terms at the end.

## Ground rule: observables, not mechanisms

The port reproduces what the player *sees*, using the engine's native vocabulary. DMG delivery
mechanisms — status-mode polling before VRAM access, the vertical-blank column flush, the sprite DMA
transfer and its HRAM trampoline, LCD-off bulk loads, the scanline interrupt that swaps scroll
registers mid-frame — do not carry into the port's code in any form. Where a mechanism produces a
visible effect, the *effect* is mapped; where it produces none, the port is not involved.

All source citations are `file:line` into the pinned disassembly. Every engine claim was verified
against the header **and** the implementation, not the guide alone.

## The headline finding

Super Mario Land is a scrolling game and uses more of the DMG PPU than Tetris did, but all of it in
fixed, enumerable ways:

- **One raster split, at a fixed line per state.** The status-interrupt source is LYC only
  (`rSTAT = $40`, written once, `bank0.asm:216`). The handler latches `rSCX` and — only when
  vertical scrolling is armed — `rSCY` (`bank0.asm:88-101`). The split line is 15 in play
  (`bank0.asm:818`), 0 during the airplane cruise (`bank0.asm:2875`), 96 during the credits
  (`bank0.asm:2959`). Vertical blank re-zeroes both scroll registers every frame (`bank0.asm:74-76`),
  which is what keeps the band above the split still.
- **A second split exists in exactly one state:** game over, where the window banner scrolls up one
  line per frame and a second interrupt switches the window off just below it (`bank0.asm:102-129`).
- **The window shows text in three states only** — pause, time up, game over — always from map
  `$9C00`, never in the bonus game, title, Daisy scenes or credits (§2).
- **Horizontal scrolling is a 32-column ring.** The playfield is streamed one column per 8 px of
  scroll into map columns 0–31 (`bank0.asm:5241-5295`); the 8-bit scroll value wraps with the map.
- **Two tile bitmaps change at runtime, both deterministically:** one animated tile
  (`bank0.asm:5454-5489`) and a 34-tile disintegration that ANDs a fixed mask sequence into VRAM
  over eight passes (`bank0.asm:2529-2564`).
- **Palettes are written three times, at boot, and never again** (`bank0.asm:229-233`): `BGP =
  $E4`, `OBP0 = $E4`, `OBP1 = $54`. No fades, no flashes.
- **8×8 sprites only**; the background-over-object priority bit is never set; no reliance on the
  ten-sprites-per-line limit (§3).
- **One tile-data addressing mode.** Background tiles always read the signed `$8800` window
  (`rLCDC` bit 4 clear in every value written); the `$8000–$8FFF` block is therefore shared between
  object tiles and background indices `$80–$FF`.

Everything on that list is either shipped engine surface or a named composition of it. No engine
work item results.

## Engine vocabulary (reference)

| Surface | What it gives |
|---|---|
| `FrameDrawState` / `DrawLayer` + `TileCell` | Z-ordered tile layers; the whole frame is recomputed and submitted per frame |
| `DrawLayer::scroll` (`LayerScroll{int x, int y}`) | Per-layer integer scroll, applied per pixel on the GPU; negative values wrap correctly |
| `TileContent::wrap` — `TileWrap::Repeat` / `Clamp` / `Blank` | Out-of-bounds sampling: toroidal, edge-smear, or transparent outside the map |
| `stencil(ShapePoints::rectangle(…), StencilMode::TransparentOutside)` | A pixel-granular rectangular clip on a layer, via a `Region` |
| `ViewportResolution::GameBoy` | 160×144 internal viewport preset |
| `Sprite` + `AssetDimensions::GameBoy8x8` | Per-sprite z / palette / flips; 8×8 is the engine default |
| `TransparentIndices::GameBoy` | Color-0 transparency, declared per uploaded sheet |
| `TileCell::atlas` + `TileCell::palette` | Every cell names its own sheet and palette; one layer mixes any number of sheets |
| Indexed atlas + `PaletteId` | Runtime palettes; shade→RGBA resolution happens at palette upload |
| `Renderer::automaticInterpolation(bool)` | Sub-tick easing of scroll and sprite position, on or off globally |
| Integer / letterbox output blit | The integer-scale and free-aspect options |
| Shader stages (`registerPostProcessStage`) | The DMG display shader |

Two engine facts shape the patterns below and are worth stating once:

- A `DrawLayer` has no clip rectangle and no origin field. (`DrawLayer::size` exists and is
  documented as per-layer dimensions, but the renderer never reads it; see the engine note at the
  end.) A layer is positioned by its `scroll` and bounded by its map extent under `TileWrap::Blank`,
  or by a `stencil` region when the boundary is not on a tile edge.
- Atlas and palette uploads append; nothing mutates or replaces uploaded pixels in place. Every
  tile the game can ever show must therefore exist in a sheet before play starts.

## Region-by-region mapping

### 1. Screen composition — a fixed band above a scrolling playfield — **Covered with pattern**

The status interrupt at line 15 latches the camera into `rSCX` (and `rSCY` when armed), so
lines 0–15 show map rows 0–1 at scroll zero — the status bar (`PrepareHUD`, `bank0.asm:891-906`,
patched in place by the score, coin, timer and lives routines) — and lines 16–143 show the
streamed playfield at the camera position (`bank0.asm:88-101`, `74-76`).
Sprites are unaffected by the split and draw over both bands.

**Pattern — band layers.** Two tile layers over the same cell data:

- *Playfield layer*, lowest z: the whole 32×32 map, `TileWrap::Repeat`, `scroll = {cameraX,
  cameraY}`.
- *Band layer*, above it: a 32×2 map holding rows 0–1, `TileWrap::Blank`, `scroll = {0, 0}`. It
  paints exactly lines 0–15 and is transparent everywhere else, so the playfield shows through
  below. Color 0 stays opaque on this sheet (the sheet is uploaded with `TransparentIndices::None`),
  which is the DMG background behavior.
- *Sprite layer*, above both.

The split line is per-state data: the band layer's `heightInTiles` is 2 in play, 12 during the
credits (split at line 96, `bank0.asm:2958-2959`), and the layer is omitted during the airplane
cruise (split at line 0, `bank0.asm:2875`). All three splits fall on tile boundaries, so `Blank`
wrap alone gives an exact result and no stencil is needed here.

**Interpolation is switched off.** The engine eases `scroll` and sprite positions between
simulation ticks by default (`LayerMotion`, `SpriteMotion`). The original advances the camera by
whole pixels per tick and never shows a fractional position; the port calls
`automaticInterpolation(false)` once at startup so every layer and sprite lands where the tick put
it. There is no per-layer opt-out, and this port needs none. (Needs a double check on the engine side that this being disabled does not affect the frame pacing on displays not 60Hz)

### 2. The window — pause, time up, game over — **Covered with pattern**

The window map is `$9C00` in every state that enables the window (`rLCDC = $C3`). Three states use
it; the level-clear flow and the Daisy dialogue draw into the background map instead.

| State | Window position | Content | Enable / disable |
|---|---|---|---|
| Pause | `WY = $85`, `WX = $60` — screen (89, 133) (`bank0.asm:821-824`) | `" ♥pause♥ "`, 9 tiles at `$9C00` (`bank0.asm:992-1000`) | toggled on the pause button (`bank0.asm:1089-1101`) |
| Time up | same position, inherited | `" time up "` (`bank0.asm:4350-4361`) | on at `bank0.asm:4363`; off when the level restarts |
| Game over | `WY = $8F → $40`, `WX = $07` — full width (`bank0.asm:4329-4332`) | `"     game  over  "`, 17 tiles (`bank0.asm:4292-4304`) | on every vertical blank (`bank0.asm:68-72`); off at the second split |

The window is a background-class layer: sprites draw over it.

**Pattern — a positioned `Blank` layer.** A tile layer holding the 32×2 window map,
`TileWrap::Blank`, `scroll = {-wx, -wy}` in screen pixels, z between the playfield and the sprite
layer. The map's pixel (0,0) lands at (wx, wy) and everything outside the map is transparent.

- *Pause / time up*: the window runs to the bottom of the screen, so the viewport edge is the only
  clip. The layer shows 8 rows of text and 3 rows of the blank second map row, exactly as the
  hardware does.
- *Game over*: the banner is visible from `WY` for **nine** scanlines — the text row plus one line
  of the blank row beneath it — because the second interrupt is armed at `oldWY + 8`, i.e. one line
  past the new `WY` after the decrement (`bank0.asm:106-116`), and it disables the window from that
  line down (`bank0.asm:122-129`). Nine is not a tile boundary, so this layer additionally carries
  `stencil(ShapePoints::rectangle(0, wy, 160, 9), StencilMode::TransparentOutside)` on its
  `regions`. `WY` decrements once per frame from 143 to 64 and holds there while the game-over timer
  runs down (`bank0.asm:108, 131-145`); while `WY ≥ 135` the second interrupt is not armed and the
  banner is a bottom sliver (`bank0.asm:111-112`) — the same stencil expresses that, since the
  viewport edge cuts it first.

### 3. Sprites — **Covered**

Always 8×8 (`rLCDC` bit 2 never set). The object buffer at `$C000` is built as a hybrid: Mario and
the four scene entities are re-composed every frame from the bank-3 composition records
(`bank3.asm:68-233`), the enemy slots are re-emitted every frame with a 20-object cap and the
remainder parked off-screen (`bank0.asm:5693-5845`), and a handful of slots — superballs, the
bounced block, score floaties, the Daisy morph, the "THE END" letters — are written once and moved
in place. Attribute usage (`bank3.asm:137-227`, `bank0.asm:5791-5822`): both flips; the object
palette select toggled by a `$FD` byte inside a composition; the priority bit **never set**.
Invisibility during the invincibility blink is an entity flag that emits an off-screen Y
(`bank0.asm:4703-4705`, `bank3.asm:96-98`), not a sprite-limit flicker; there is no per-frame
reordering anywhere.

Each visible object becomes a `retropp::Sprite` (`GameBoy8x8` is the engine default), flips as
`flipX`/`flipY`, the palette select as a per-sprite `PaletteId` (two object palettes uploaded once),
color-0 holes via `TransparentIndices::GameBoy` on the object sheet, and buffer order mapped to `z`.
Objects the original parks off-screen are simply not emitted. The 20-object enemy cap and the
40-object buffer are game-logic limits and port with the game logic; the engine imposes none.

One consequence of the shared `$8000–$8FFF` block: the same 256 bitmaps serve as object tiles
(color 0 transparent) and as background tiles `$80–$FF` (color 0 opaque). Transparency is a
property of the uploaded sheet, so the extractor writes that block into **two** sheets — one
uploaded with `TransparentIndices::GameBoy` for sprites, one with `TransparentIndices::None` for
tile cells. Same bytes, two handles.

### 4. Playfield streaming — the 32-column ring — **Covered**

The level is decoded one 16-tile column at a time into a stage buffer during the main loop and
written into map rows 2–17 at the next vertical blank (`bank0.asm:5116-5295`). The write column
advances through `0..31` and wraps (`bank0.asm:5247-5252`); a new column is staged every 8 px of
camera travel (`bank0.asm:5098-5113`). Twenty-seven columns are preloaded at level start
(`bank0.asm:1140`; twenty for underground and the menu, `bank0.asm:1133-1139`), so the write head
runs six full columns ahead of the right screen edge and the column being overwritten is five
columns behind the left edge — the visible 20 (+1 partial) columns never expose an unloaded column.
The 8-bit camera value wraps at 256 px, exactly the map width, so ring index and scroll stay in
lockstep with no extra bookkeeping.

The observable is "a 32-column map whose cells the game rewrites as the camera moves, sampled with
wrap-around." That is `TileContent` over a 32-wide cell array with `TileWrap::Repeat` — toroidal on
both axes via floor-mod, negative scroll included — and `scroll.x = cameraX`. The port keeps the
map as game state, writes columns exactly when the original does (the timing ports with the level
streaming logic), and submits the whole map every tick; the renderer skips the upload whenever the
cells have not changed, so scrolling a static map costs nothing. The autoscroll levels differ only
in what drives the camera (`bank1.asm:43-76`: +1 px every other frame), not in the ring.

### 5. Per-world tile sets — **Covered**

World 1 reloads the full background and object tile blocks with the LCD off
(`bank0.asm:870-889`); worlds 2–4 patch two ranges of the world-1 set — 61 enemy tiles at
`$8A00–$8DCF` and 63 backdrop tiles at `$9310–$96FF` — from per-world offsets (`bank0.asm:2021-2051`,
tables at `2082-2084`). The menu loads a fifth, separate set (`bank0.asm:466-490`).

Engine uploads are load-time operations: each `uploadAtlas` appends a new sheet and rebuilds the
whole atlas store, so they belong at startup, not at a level boundary. The port therefore uploads
every world's composite set (world-1 base with the per-world patches applied — the extractor writes
one sheet per world, plus the menu sheet) once, and selects per cell through `TileCell::atlas`.
"Reload the tile set at world change" becomes "point the cells at a different sheet," which is
free. Five sheets resident for the whole session is trivial.

### 6. Tile bitmaps that change at runtime — **Covered with pattern**

Two, both fully deterministic:

- **The animated tile.** Background tile `$5D` has the odd (plane-1) byte of each of its eight rows
  rewritten every 8 frames, alternating between the world's own bitmap (saved to WRAM at load time)
  and a per-world 8-byte frame from ROM (`bank0.asm:5454-5489`, frames at `7521-7525`) — a
  16-frame period. It runs only in the six levels flagged at `bank0.asm:5493` (1-3, 2-1, 2-2, 3-2,
  3-3, 4-2): the flickering candle and the waves.
- **The Tatanga disintegration.** In the final fight, 544 bytes of VRAM — 19 tiles at `$8DD0–$8EFF`
  and 15 at `$9690–$977F` — are ANDed in place with a rolling mask, one pass every 32 frames for
  eight passes (`bank0.asm:2529-2564`). The mask sequence is fixed (`$BF, $E7, $EC, $8D, $A1, $24,
  $84, $80`; `bank0.asm:2557-2561`) and rotates right one bit per byte, and since every tile is 16
  bytes the rotation phase is identical for every tile. Bits only ever clear, so the eight states
  accumulate.

Neither needs a mutable atlas. Both are **pre-baked tile variants selected per cell**: the
extractor emits the two frames of tile `$5D` for each world, and the nine states (untouched plus
eight passes) of each of the 34 disintegrating tiles for world 4, into the extracted sheets; at
runtime the cell's `TileCell::tile` steps through the variants on the original's cadence. Cells are
per-frame data and the upload is elided when nothing changed, so this costs one cell rewrite per
step. `Animation` / `AnimationPlayer` is the engine's wrapper for exactly this and is the natural
home for the candle and the waves.

### 7. Vertical scroll under a fixed band — shake and credits — **Covered**

`wScrollY` reaches `rSCY` only below the split, and only in two states (`bank0.asm:97-101`, gate
at `$C0DE`): the Tatanga fight, where the playfield judders ±4 px every 4 frames while the tiles
disintegrate (`bank0.asm:2515-2528`), and the credits, where two lines of text written into map rows
18 and 20 — below the visible area — are scrolled up through the band under the line-96 split, 1 px
every 4 frames, held, then scrolled out (`bank0.asm:3006-3107`). The status bar, and the airplane
and clouds above the credits split, never move; sprites are unaffected by `rSCY`.

This is the band pattern of §1 with `scroll.y` set on the playfield layer alone. The full 32×32
map with `Repeat` wrap makes the credits rows and the blank row that appears under a 4-px shake
come out exactly as on hardware. With interpolation off (§1) the shake snaps as the original does.

### 8. Palettes and shade presentation — **Covered**

Three fixed palettes for the whole game: `BGP = %11100100` (identity), `OBP0 = %11100100`, `OBP1 =
%01010100` — the second object palette renders colors 1–3 all as light grey, which is how lighter
sprite variants are produced from the same bitmaps (`bank0.asm:229-233`). Three `PaletteId`s
uploaded at startup; the cell and sprite records name them. The four-shade output ramp, the
integer-scale and free-aspect options and the DMG display shader ride the palette upload, the output
blit and the shader-stage surfaces, as in the sibling port.

### 9. Transition blanks — LCD off during loads — **Covered**

Every screen change turns the LCD off, draws the whole screen, and turns it on again (ten sites;
e.g. `bank0.asm:447-585`, `795-2068`, `7161-7192`). The observable is a brief white screen between
scenes, which maps to submitting a frame with no layers or a whole-frame fill — or simply to
submitting the new frame. Whether the white gap is part of the preserved timing is a per-transition
decision for the screen-flow contracts.

### 10. Non-renderer mechanisms — no engine surface required

For completeness: the mode-0 polling before every VRAM access (`WAIT_FOR_HBLANK`, `macros.asm:1-6`
and its open-coded copies); the doubled tile read in `LookupTile` (`bank0.asm:173-180`), a defense
against a timer interrupt landing mid-poll and already recorded as a quirk in `DESIGN.md` §6; the
sprite DMA trampoline with its two-byte over-copy into HRAM padding (`bank0.asm:278-286`,
`7504-7512`); the `rLY` poll before the very first LCD-off at boot (`bank0.asm:224-226`); the
vertical-blank budget flag that defers the score display by one frame whenever a column is flushed
(`bank0.asm:5293-5294`, `7447-7449`) — an ordering quirk that ports with the HUD logic, not the
renderer; and the bonus game's ungated tile-map writes, including a prize-erase loop that runs twice
(`bank2.asm:843-878`), which land on hardware only because they are retried. All are mechanisms
whose observables are accounted for above, or that have none.

## Engine work items

**None.** Every renderer-visible behavior in Super Mario Land maps onto the engine's shipped
surface. Two compositions carry the fidelity-sensitive parts and are named above so the rendering
design uses them and nothing else: band layers under `TileWrap::Blank` (§1, §2, §7) and a `stencil`
rectangle for the one clip that is not on a tile edge (§2, game over).

Watch items carried to the work that will resolve them, none of which forecasts engine work:

- The extractor's sheet layout owes: one background sheet and one object sheet per world (§3, §5),
  the two frames of tile `$5D` per world and the nine disintegration states of 34 world-4 tiles
  (§6).
- Per-transition white-blank preservation (§9) — a contract decision.
- Pixel-exact regression tests read the composed frame back at compose scale 1; the windowed run
  composes at the window's integer scale when interpolation is on, so the golden path is the
  interpolation-off path this port uses anyway.

**Engine note (filed, not blocking).** `DrawLayer::size` is documented in the header and guide as
independent per-layer dimensions, but the renderer never reads it: a tile layer always rasterizes
over the whole viewport. A reader would reach for it as the status-bar clip; the correct surface is
`TileWrap::Blank` or a `stencil` region, as used above.

## Verdict summary

| # | Behavior region | Verdict |
|---|---|---|
| 1 | Screen composition (status band above a scrolling playfield; split at line 16 / 96 / 0) | Covered with pattern — band layers |
| 2 | Window (pause, time up, scrolling game-over banner with its second split) | Covered with pattern — positioned `Blank` layer (+ stencil for game over) |
| 3 | Sprites (8×8, flips, two fixed palettes, no priority bit) | Covered |
| 4 | Playfield streaming (32-column ring, 27-column preload, scroll wrap) | Covered |
| 5 | Per-world tile sets (full and partial reloads) | Covered |
| 6 | Runtime tile-bitmap changes (animated tile, disintegration masks) | Covered with pattern — pre-baked variants |
| 7 | Vertical scroll under a fixed band (shake, credits) | Covered |
| 8 | Palettes and display options | Covered |
| 9 | Transition blanks (LCD off) | Covered |
| 10 | Non-renderer mechanisms | No renderer surface needed |

Zero engine work items. The renderer side of this port is consumption of shipped engine surface,
with two named compositions.
