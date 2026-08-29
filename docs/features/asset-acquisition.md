# Asset Acquisition

**Date:** 2026-08-28; revised the same day after the data audit (four content families, level file
contents, the sound-driver span, text)
**Status:** Designed — nothing implemented yet

How the game gets the graphics, levels, sound, and text it needs, without any copyrighted content
entering this repository or a shipped build.

## Concept

The engine loads each content family from one canonical path under the asset root:

| Family | Path | What it holds |
|---|---|---|
| Tile graphics | `assets/gfx/default/` | The four worlds' background and enemy tile sets, the common sets, the menu sets, and the animated background tile's frames, as greyscale PNGs |
| Levels | `assets/levels/default/<level>.bin` | One file per level — the twelve stages, the start-menu backdrop, and the end-of-game hangar. Each carries the level's screen list, its column blocks, its warp pipes, its block contents, and its enemy placements |
| Sound driver | `assets/audio/default/sound_driver.bin` | The sound driver and every song and effect, as one span at the address it came from |
| Text | `assets/text/default/strings.bin` | Every string the game displays — the HUD rows, "game over", "time up", the pause banner, the ending dialogue, the credits — as indexed tile-index strings |

The asset root is the player's own data directory for an installed game
(`~/Library/Application Support/<org>/<app>`, `%APPDATA%\<org>\<app>`, `$XDG_DATA_HOME/<org>/<app>`)
and the project tree for a development build still inside that tree. The paths are the same in
both.

**No pack system.** No swappable packs, no pack discovery, no manifest, no fallback chain. The
engine reads the canonical paths or it reports an error.

## Design decisions

### One route, every environment

The player's ROM is the sole source of every content family. The game's first-start flow — a
native file dialog, verification, extraction, then normal startup — is how a player populates the
paths, and it is also how a developer and the CI runners do: each supplies a ROM of their own to the
same extractor. There is no script that copies content out of the disassembly checkout.

### What is content and what is not

Graphics, levels, music, and text are authored expression and are extracted. Level design is taken
whole: the tile columns, the warp destinations, what each block holds, and where every enemy stands
are the level. What compiles into the binary is mechanics — physics curves, timers, scoring, enemy
behaviour parameters, sprite layouts, tile classes — none of which is a byte of art, audio, layout,
or text. The line is drawn once for the whole data layer, not per table.

### Level files

Each level file is the port's own format, pinned in a contract before the extractor is written. It
is not a ROM image: the extractor decodes the level's screen list, resolves every column block it
references, and appends the level's warp table, block-item table, and enemy placement table, so the
game's column-streaming code reads one self-contained file per level.

### The sound driver travels as one span

The driver reaches its music and effect data by absolute address, so the data cannot be separated
from the code that reads it. The whole region travels together and the engine's audio system places
it at the base it came from and runs it there.

### Text is a fixed file, not a pack

The text corpus is small — about fifteen strings — and is read with ordinary file I/O through an
indexed loader. It does not go through any pack machinery.

### First-start sequencing — port-side only

When the game starts and required content is absent, it asks for the ROM and gets on with it — the
first-start model players know from Ship of Harkinian.

1. `main()` checks every canonical path for content.
2. Anything missing → show the platform's file-selection dialog (SDL's, called directly), extract
   from the ROM the player chooses, write every family.
3. Proceed into normal engine construction and asset loading — the same code path every later
   launch takes.

**Never a silent failure, never placeholder content, never a bundled fallback asset, and never a
bare error pointing at a tool the player has to go and find.** The flow states plainly that the ROM
is only read — never copied, moved, or altered.

Extraction refuses anything that is not the expected ROM — exact size (65,536 bytes) and SHA1
(`418203621b887caa090215d97e3f509b79affd3e`), checked before a byte is written — and decodes every
output in memory before the first file lands, so no failure leaves a half-populated install.

### The distributable ships empty asset directories

The distributable build target empties every `default/` directory before packaging, retaining the
`.gitkeep` placeholders so the structure ships. A packaging check fails if anything else is present.

## Implementation details

Nothing is implemented yet. The planned shape follows Kirpich's:

- `src/assets/presence.{h,cpp}` — the required-asset manifest and the presence check.
- `src/assets/first_start.{h,cpp}` — the first-start flow.
- `src/assets/extract.{h,cpp}` — the extractor: the ROM identity gate, the tile decode, the level
  decode, the sound driver span, the text strings, and the writes.
- `src/assets/png_writer.{h,cpp}` — greyscale PNG serialization.
- `assets/{gfx,levels,audio,text}/default/.gitkeep` — directory placeholders.

The extraction table — which bytes of the ROM each output comes from — is generated from the
disassembly and the ROM and lives beside the extractor as data; the offsets and decodes are pinned
in the contracts for each family.

## Open questions

- **Level file format.** Decided at the level unit's kickoff, in its contract.
- **Demo recordings.** The three attract-mode input recordings are input sequences, not authored
  expression; whether they compile in (as Kirpich's do) or ride in the level directory is an open
  ruling.
