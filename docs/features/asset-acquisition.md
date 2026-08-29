# Asset Acquisition

**Date:** 2026-08-28
**Status:** Designed — nothing implemented yet

How the game gets the graphics, level data, and ROM byte spans it needs, without any copyrighted
content entering this repository or a shipped build.

## Concept

The engine loads each content family from one canonical path under the asset root:

| Family | Path | What it holds |
|---|---|---|
| Tile graphics | `assets/gfx/default/` | The four worlds' background and enemy tile sets, the three common sets, and the two menu sets, as greyscale PNGs |
| Sound driver | `assets/audio/default/sound_driver.bin` | The music-and-effects region of bank 3, one span, unchanged |
| Levels | `assets/levels/default/` | The twelve levels' column data and enemy placement tables, one file per level |

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

This is stricter than the common practice of building level data into the binary. Level data is
authored expression in exactly the sense that graphics and music are, and the twelve levels ship
extracted, never compiled in. What stays compiled in is mechanics — physics constants, timers,
scoring, enemy behavior parameters — which are not copyrightable and are verified against the ROM by
the tests.

### Level data is extracted, not compiled

The twelve levels live across banks 1, 2, and 3 as column-oriented tile data with per-level enemy
placement tables. The extractor decodes each level to its own file; the game's column-streaming code
reads those files at level load. The file format is the port's own, pinned in a contract before the
extractor is written, and is not a ROM image.

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
  decode, the sound driver span, and the writes.
- `src/assets/png_writer.{h,cpp}` — greyscale PNG serialization.
- `assets/{gfx,audio,levels}/default/.gitkeep` — directory placeholders.

## Open questions

- **Level file format.** Decided at the level-data unit's kickoff, in its contract.
- **Sound driver span extent.** The music region starts at bank 3 `$6F98`; the driver code that
  reads it and the entry the timer interrupt calls (`$7FF0`) are in the same bank. The exact span
  and its base are pinned at the audio unit's kickoff.
