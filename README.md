# Uncanny Plumber

A native reimplementation of **Super Mario Land** for the Game Boy (DMG), running as ordinary desktop
software on Windows, macOS, and Linux. Uncanny Plumber is built on the
[Retro++ engine](https://github.com/RetroPlusPlus/Engine) and reproduces the original cartridge's
observable behavior — the same game, given the same inputs — without emulating the hardware and
without translating the assembly. The player's own cartridge supplies the graphics, the levels, and
the sound.

**Status: foundation.** The build and test harness stand; no game code yet.

## Architecture

The game logic is ordinary C++: the cartridge's mechanics tables are `constexpr` arrays verified
against the ROM, its RAM layouts are structs, its code paths are functions. Nothing simulates the
Game Boy's PPU, memory mapper, or interrupt hardware.

Two subsystems are exceptions, and only these two run original machine code on an emulated CPU
inside the engine:

- **Randomness.** The original reads the DMG's divider register, which ticks independently of the
  program counter; the values depend on cycle-exact timing and cannot be reproduced by
  re-implementing the arithmetic.
- **Audio.** The ROM's sound driver programs the audio hardware on a cycle-driven cadence; faithful
  chiptune output requires running that driver against an emulated audio unit.

### Repository layout

| Path | What it is |
|---|---|
| `src/`, `include/uncannyplumber/` | Port source and public headers |
| `tests/` | GoogleTest suite |
| `engine/` | [Retro++](https://github.com/RetroPlusPlus/Engine) engine submodule — brings SDL3 and SameBoy with it |
| `assets/{gfx,audio,levels}/default/` | Where a development build reads its extracted content; contents are generated locally and never committed |
| `docs/` | Design context and feature documentation |
| `tools/` | Development tooling |

The [kaspermeerts/supermarioland](https://github.com/kaspermeerts/supermarioland) disassembly is the
derivation reference. It is read during development as a sibling checkout outside this repository —
it is not a submodule, and the build never depends on it.

### Building

Requires CMake 3.28+, a C++20 compiler (GCC 13+ / Clang 16+ / AppleClang 15+ / MSVC 19.38+), and
recursive submodules:

```sh
git clone --recursive https://github.com/etroimcasso/UncannyPlumber.git
cd UncannyPlumber
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build
```

The build defaults to a lean Release configuration.

## Content and licensing posture

Uncanny Plumber distributes no playable copyrighted content: no ROM data, no extracted assets, no
level data, nothing in any build artifact. Graphics, levels, and the sound driver are extracted
locally from a Super Mario Land (World, Rev A) ROM the player legitimately owns, into the player's
own user directory. `.gitignore` bans ROM extensions and extracted content tree-wide, and the
packaging step verifies the shipped artifact carries neither. See
[`docs/features/asset-acquisition.md`](docs/features/asset-acquisition.md).

## License

Uncanny Plumber is licensed under the [GNU Affero General Public License v3.0](LICENSE), matching the Retro++ engine's
open-source license (the engine itself is dual-licensed AGPL-3.0 / commercial). The upstream
disassembly is published without a license.

Super Mario Land is a trademark of Nintendo. This project is unaffiliated with and unendorsed by
Nintendo, distributes no copyrighted content, and requires the user's own ROM.
