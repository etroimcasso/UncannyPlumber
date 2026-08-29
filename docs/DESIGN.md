# Uncanny Plumber — Design Context

Design decisions locked 2026-08-28 at inception. This document is the authoritative record of
project intent, the behavior-preservation contract, and the constraints every implementation
decision is measured against.

*Uncanny Plumber* is the project's name; the C++ namespace, the CMake project, and every target
follow it (`uncannyplumber`, `uncannyplumber-lib`, `uncannyplumber-tests`).

## 1. What this project is

A behavior-preserving native C++ port of the Game Boy (DMG) release of Super Mario Land, built as a
consumer of the [Retro++ engine](https://github.com/RetroPlusPlus/Engine). The port reads the
[kaspermeerts/supermarioland](https://github.com/kaspermeerts/supermarioland) disassembly for
intent, mechanics, and data, and writes idiomatic modern C++ against the engine's surface — run
loop, renderer, input, audio, and CPU virtualization host. SDL3 with the SDL_GPU backend arrives
transitively inside the engine.

Behaviors that depend on cycle-exact execution of the original machine code — the divider-register
reads the game uses as its randomness source, and the original music and sound driver — run on the
engine's ROM-less SM83 virtual machine host (§10). Everything else is reauthored natively.

The shipped artifact is a game binary plus tooling that runs on macOS, Linux, and Windows and
reproduces the original ROM's observable behavior given the same inputs.

This is the same shape as [Kirpich](https://github.com/etroimcasso/Kirpich), the engine's reference
consumer; where this document is silent, Kirpich's `docs/DESIGN.md` is the precedent.

## 2. What this project is NOT

- **Not an emulator.** No full-system emulation, no PPU state machine, no memory-bank registers,
  no interrupt emulation, no memory mapper, no boot ROM. The narrow exception is the routine set
  named in §10 — a surgical tool, not a system emulator.
- **Not a hosted cartridge.** The engine can boot a whole ROM in its VM and run it; this port does
  not use that. The original ROM is read once, by the extractor, on the player's machine. It is
  never loaded and run.
- **Not a mechanical assembly-to-C++ translation.** Data tables become `constexpr` arrays; RAM
  structures become C++ structs; code paths become idiomatic functions; hardware-register effects
  are expressed at engine and renderer level. The virtualized routines execute original bytes as
  extracted spans handed to the engine's VM — never as translated C++.

## 3. Upstream and pin

| Field | Value |
|---|---|
| Upstream repo | `https://github.com/kaspermeerts/supermarioland.git` |
| Pinned commit | `618d00ed6c330928e106719533c6e294ae5d5726` (2022-09-30) |
| Upstream license | None — the disassembly is published without a license file |
| Original ROM | Super Mario Land (World) (Rev A) — SHA1 `418203621b887caa090215d97e3f509b79affd3e`, 65,536 bytes |

The pinned disassembly checkout lives as a sibling directory (`../original-src/`) outside this
repository and is treated as a read-only reference. It is never modified by port work, and the
build never depends on it.

**The disassembly is incomplete.** Upstream's own estimate: bank 0 about three quarters
disassembled, bank 1 a third, banks 2 and 3 half, and 46 of 127 high-RAM bytes named. The
undisassembled regions are `INCBIN`'d from the original ROM in the upstream build. For this port
that means the contract source has gaps: routines that live in those regions must be
reverse-derived from the ROM bytes themselves (disassembled locally, in this repository's private
working set, never upstreamed) before they can be ported. Mapping those gaps is a foundation
deliverable, not something discovered per feature.

## 4. Hardware scope

Fixed by the ROM's actual properties — non-negotiable.

| Field | Value |
|---|---|
| Console | DMG (original Game Boy) |
| Memory mapper | MBC1, ROM banking only — 4 × 16 KB banks; bank 0 fixed, banks 1–3 switched |
| ROM size | 64 KB |
| Save RAM | None |
| Real-time clock | None |
| Super Game Boy code | None |
| Audio | Standard 4-channel sound unit (two square, wave, noise) |
| Display | 160 × 144, 4-shade greyscale |
| Interrupts used | V-blank (game frame), LCD STAT (mid-frame display change), Timer (drives the sound driver) |

Two things here are new relative to Kirpich. **Banking**: originally banked data, once extracted, is
just data — there is no bank-switch logic in port code (§11), but the extractor and the data audit
must know which bank every table lives in. **The timer-driven sound driver**: the original's music
runs from the timer interrupt, not from the frame, so the driver's cadence relative to the game
frame is a hosting question for the audio unit, resolved at the audio unit's kickoff.

## 5. Behavior-preservation premise

**The port must produce the same observable behavior as the original ROM for any given input
sequence and RNG state.** This is the governing design rule. Every implementation decision is
evaluated against it. When something in the disassembly looks suboptimal, accidental, or buggy, the
default is to preserve it. Improvements are out of scope.

Concretely: a frame-by-frame capture of the port playing the same inputs from the same power-on
state must be indistinguishable from the same capture taken from the original ROM. Sprite positions,
tile updates, scroll, scoring, enemy behavior, audio output, and RNG state evolution all match at the
observable boundary.

Behaviors identical by construction rather than by re-implementation:

- **Randomness.** The game reads the DMG divider register at several sites (enemy behavior, bonus
  game, and others — the full list is pinned at the audit). The register ticks at 16384 Hz
  independently of the program counter; the value read depends on cycles since power-on. Native
  re-implementation cannot reproduce it, so the divider is advanced on the engine's virtual SM83 at
  the true rate and read from there.
- **Music and sound effects.** The sound driver writes audio-channel registers on a cycle-driven
  cadence. Reproducing the output requires both CPU and audio-unit cycle accuracy, so the driver runs
  on the engine's virtual SM83 against its emulated audio unit.

Departures from behavior preservation appear only under the named options in §7.

## 6. Quirks preserved

Each of these is a documented behavior of the original and is non-negotiably part of the port's
correctness contract. The list is seeded from the disassembly's own annotations and grows at the
audit and during porting; every addition lands in the same session as its discovery.

| Quirk | Description |
|---|---|
| Divider-register randomness may be degenerate | The disassembly notes at one site that because the music code is linked to the timer interrupt, the divider register "might always be 1" when read. Whether that is so is a property of the original's timing, and the port reproduces it by reading the divider from the VM rather than deciding. |
| Timer interrupt overlaps the serial vector | The timer handler's code runs into the serial-interrupt vector mid-instruction. Serial is unused, so nothing observes it; preserved as an equivalence. |
| Demo recorder present but unreachable | A routine records the player's inputs as a demo when a flag holds a value nothing in the game ever writes. Preserved as dead-but-present code, the way Kirpich carries its own recorder. |
| Genkotsu's fist ends the level | Touching the Genkotsu fist tile from the side runs the level-complete path rather than injuring Mario. Preserved. |
| Tile reads are doubled | The original reads a background tile twice and combines the reads to survive a display-timing hazard. The port has no such hazard; a single read is the equivalence. |
| Start-up clears memory that is not there | Initialisation wipes the cartridge-RAM address range on a cartridge with no RAM, over-runs the object-attribute and high-RAM clears by a byte, copies the sprite-transfer routine two bytes long, and copies Mario's initial entity image one byte long. Every one is harmless on hardware; each is preserved as an equivalence with its reachability argument in the boot contract. |
| A fifth world's pass-through tiles | The table of tiles Mario can stand on but not bump has five entries for four worlds. The fifth is unreachable; preserved. |

## 7. Options (user opt-in, off by default)

These are the port's departures from the original beyond the display and speaker boundary. Each is
user-facing, opt-in, off by default, and composes with the others in its category.

### Display

| Option | Description |
|---|---|
| Integer scale | Render the 160×144 framebuffer at integer multiples. Preserves pixel-perfect geometry. |
| Free-aspect output | Non-integer scaling and stretching to fit arbitrary window sizes. Cohabits with integer scale; one is active at a time. |
| DMG display shader | Reproduces the original LCD's optical character — greenish tint, ghosting, dot grid. |
| Colour palettes | Alternative four-shade ramps for the greyscale output, selectable in-game. |

**CRT shaders are explicitly not offered.** The Game Boy was never displayed on a CRT.
**Pixel-art upscaling shaders are not offered** — the engine presents at integer multiples with no
interpolation, which is what keeps the art crisp.

### Audio

| Option | Description |
|---|---|
| Anti-channel-stealing | Off by default, which preserves the original's channel stealing exactly. When enabled, music and effects are hosted as separate driver instances so an effect never costs the music a voice. |

### Persistence (always-on)

The original keeps its top score in RAM only. The port persists the top score across launches,
always on, through the engine's durable save store. It changes only what survives a power cycle,
never what the game produces within a session.

Further options (for example, the enhancements Kirpich's fixes screen offers) are proposed as
amendments to this document, never added silently.

## 8. Technical decisions

Each is a locked decision — changing one means revisiting this document, not just an
implementation file.

| Decision | Value |
|---|---|
| Approach | Modern C++ reimplementation consuming the Retro++ engine. Read the disassembly for intent, mechanics, and data; write idiomatic C++ against the `retropp::` surface. Not mechanical translation. Not a hosted cartridge. |
| Platform layer | The Retro++ engine, consumed as a git submodule via `add_subdirectory(engine)` + `retropp::engine`, pinned at `594774c`. SDL3 with the SDL_GPU backend is engine-internal; the port declares no SDL3 of its own. Direct SDL calls are permitted where the engine has no opinion (the first-start ROM picker is the expected one). Engine gaps are filed upstream, never shimmed port-side. |
| C++ standard | C++20 |
| Build | CMake 3.28+ with Ninja; Release by default, dead-stripped and symbol-stripped |
| Tests | GoogleTest |
| Logging | spdlog, used directly — no wrapper |
| CPU virtualization | The engine's VM host (`retropp::Vm`), SameBoy-backed and ROM-less: routines are registered as extracted byte spans and called as typed C++ functions. No game ROM is ever loaded into an emulator. |
| Simulation rate | True DMG frame rate — 70,224 CPU cycles at 4,194,304 Hz, i.e. 59.7275 Hz — via the engine's Game Boy timing profile. |
| Threading | Single-threaded main loop on the platform thread. Virtual-machine calls are synchronous from the game's perspective. |
| Hardware target | DMG, MBC1 ROM banking (no RAM banking), no SRAM, no RTC, no SGB |
| Hardware-register variables | **None in port code.** `rLCDC`, `rSCX`, `rSCY`, `rDIV`, `rIE`, `rIF`, `rNR10`–`rNR52` and friends do not exist as variables anywhere under `src/`. Their effects are expressed at engine and renderer level. Registers appear only inside the engine's VM boundary — in a routine's registration, never at a call site. |
| Audio | Chiptune only. The engine's audio system hosts the ROM's sound driver on its internal VM and produces PCM into the engine mixer. No audio-file replacement backend. |
| Asset posture | Single canonical path per content family: `assets/gfx/default/`, `assets/levels/default/`, `assets/audio/default/`, `assets/text/default/`. No swappable packs, no manifest, no fallback chain. The player's ROM is the sole source of every content family, in every environment — development, CI, and player alike populate by extraction. |
| Content boundary | Graphics, **level data** (columns, screens, warp pipes, block contents, enemy placements — the start-menu backdrop and the ending hangar included), the sound driver with its music and effects, and **text** are authored expression: extracted at runtime, never committed, never compiled in. Mechanics (physics curves, timers, scoring, enemy behaviour scripts and attribute tables, sprite layouts, tile classes) are compiled in as `constexpr` data. Classified once for the whole data layer (2026-08-28), not per table. |
| License posture | AGPL-3.0 (`LICENSE` at the repo root), matching the Retro++ engine's open-source license — the whole distributed build is one AGPL combined work. The engine is dual-licensed (AGPL-3.0 / commercial); the upstream disassembly is published without a license. |
| Repository posture | Standalone repository. The disassembly is a sibling checkout outside the tree; this is not a fork of it. |
| Public repository | `https://github.com/etroimcasso/UncannyPlumber`, public from its first push. `docs/` and `README.md` are the public face; the private working set is gitignored. |

## 9. Asset posture

The engine loads each content family from one canonical path under the asset root — the player's
own data directory for an installed game, the project tree for a development build. Directories are
committed via `.gitkeep`; their contents are gitignored.

**One route, every environment.** The game's first-start flow asks for the ROM through a native
file dialog, verifies its size and SHA1, decodes every output in memory — graphics, the fourteen
level files, the sound driver span, the text strings — and writes the layout atomically through
the engine's file store. Developers and CI populate the same way with a ROM of
their own; there is no script that copies content out of the disassembly. A missing ROM is a
provisioning failure, never a silent skip.

**Shipping:** the distributable build target empties every `default/` directory before packaging,
and a packaging check fails if anything but `.gitkeep` is present.

Full design: [`features/asset-acquisition.md`](features/asset-acquisition.md).

## 10. Virtualized routines and audio architecture

| Routine | Throttling | Why it is virtualized |
|---|---|---|
| Divider-register reads (the game's randomness source) | Host speed — the divider is advanced by the exact cycle count each tick, so the value read is identical at any host rate | A byte-identical read requires the divider's tick model relative to elapsed cycles. Re-implementation cannot reproduce it. |
| Music and sound driver | Throttled to 4.194304 MHz; the audio sink consumes output at the device's real-time sample rate | Chiptune fidelity requires CPU and audio-unit cycle accuracy plus correct alignment to the output sink rate. |

**Backend.** [SameBoy](https://sameboy.github.io), owned by the engine — the port never touches it
directly.

**Randomness.** The port owns its divider routine as a port-side VM routine, per Kirpich's
`src/vm/random.asm` pattern: a small SM83 routine registered with the engine, with the divider
advanced per tick. The exact set of divider-reading sites and what each does with the byte is pinned
at the audit; the arithmetic around the byte is native C++.

**Audio.** The sound driver — the bank-3 music region and the routines that drive it — registers
with the engine's audio system, which hosts it on an internal VM at the correct cadence. The
original drives it from the timer interrupt; how that cadence is expressed to the engine's hosting
(which is frame-verb driven in Kirpich) is the audio unit's opening question and may produce an
engine request.

## 11. Hard prohibitions

1. **No full-system emulation outside the VM.** The main loop, rendering, input, and game logic run
   as native C++. The VM exists only for the routines named in §10.
2. **No hardware-register variables in native port code.**
3. **No PPU state machine anywhere.** Pixel output is the renderer's responsibility. Mid-frame
   display changes the original makes from the LCD STAT interrupt are expressed as renderer regions
   or layers, not as a scanline emulation.
4. **The audio state machine lives only inside the VM.**
5. **No bank-switching logic.** Extracted data is just data. Bank membership is an extractor and
   audit concern only.
6. **No copyrighted content in shipped artifacts.** Graphics, audio, level data, and the VM's byte
   spans arrive by user-runtime extraction, never in the binary or the repository.
7. **Never modify the upstream disassembly.** Gap-filling disassembly of the undisassembled regions
   is done in this repository's private working set, never in `../original-src/`.
8. **No decompiler or transpiler output in native code.**
9. **No hosted-cartridge execution.** The engine's ability to run a whole ROM is not used, not even
   as a development oracle.

## 12. What must be preserved

- **Behavior preservation** — same observable outputs given the same inputs (§5).
- **The quirks list** (§6), as it grows.
- **The virtualized routine list and the VM backend** (§10).
- **The content boundary** (§8) — level data is extracted content, not compiled-in data.
- **Anti-channel-stealing ships as an option**, off by default (§7).
- **All display options compose** (§7).
- **No copyrighted content ships** — enforced structurally (§9).
- **The hard prohibitions** (§11).
- **Two deliberate reductions:** chiptune-only audio and a single asset path per family.

Anything conflicting with the above is raised as an amendment to this document, not implemented
silently.
