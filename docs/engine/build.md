# Build & run

How Uncanny Plumber is put together, what the targets are, and how to build, run, and test it.

## Requirements

- **CMake 3.28+** and a generator. Ninja is what the project is developed against.
- **A C++20 compiler.** The build enforces floors and fails at configure time below them:
  GCC 13, Clang 16, AppleClang 15, MSVC 19.38 (Visual Studio 2022 17.8).
- **Git**, with submodules — the engine is one, and it has its own nested.

`ccache` is used automatically if it is on `PATH`. It is skipped under MSVC, which it does not
integrate with.

macOS builds target **arm64 only**, with a deployment target of **11.0**. Both are set before
`project()`, because that is when the compiler is probed and an architecture list or deployment
target arriving afterwards is only half-applied.

## First build

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja
cmake --build build --parallel
ctest --test-dir build
```

The submodule step is not optional: configure stops with an explicit message if
`engine/CMakeLists.txt` is absent, because a missing engine otherwise surfaces as a wall of
unrelated errors.

Executables land at the build root rather than under a per-target subdirectory:

```sh
./build/UncannyPlumber            # macOS: open build/UncannyPlumber.app
```

To play, the game also needs graphics, audio, level and text data, none of which ship with the
source — they are decoded from a player's own ROM on first start. See
[`../features/asset-acquisition.md`](../features/asset-acquisition.md).

## Targets

| Target | What it is |
|---|---|
| `uncannyplumber-lib` | Static library holding everything except `main()`. Links `retropp::engine` and `spdlog::spdlog_header_only` publicly, so anything linking it sees both surfaces. Its `include/` and `src/` directories and the `UNCANNYPLUMBER_VERSION` definition are public too. |
| `uncannyplumber` | The executable. `main.cpp` plus `uncannyplumber-lib`. Its `OUTPUT_NAME` is `UncannyPlumber`. |
| `uncannyplumber-tests` | GoogleTest runner, registered with CTest. |

`main()` is deliberately kept out of the library so the tests can link the whole port without
pulling in a second entry point. The CMake target names stay lowercase because that is what the
build refers to everywhere; the artifact a player sees is a proper noun.

Test sources are globbed with `CONFIGURE_DEPENDS`, so **adding a `.cpp` under `tests/` requires no
CMake edit** — it is picked up on the next build. Cases register with CTest individually via
`gtest_discover_tests`, which is why `ctest` lists them by name rather than as one opaque binary.
The suite links `uncannyplumber-lib` the same way the program does, so a linkage defect in the
program is reachable from the suite.

## Dependencies

| Dependency | How it arrives |
|---|---|
| Retro++ engine | `engine/` git submodule, `add_subdirectory` |
| SDL3 | Transitively, from the engine (`engine/third_party/sdl`) |
| SameBoy | Transitively, from the engine (`engine/third_party/sameboy`) |
| spdlog | `FetchContent`, v1.15.3, header-only |
| GoogleTest | `FetchContent`, v1.15.2, gmock off |

**The port must never declare SDL3 itself.** It arrives through the engine, and a second provider
of the `SDL3::SDL3` target is a configure-time error. The port may *call* SDL directly where the
engine has no opinion, but declaring it is a different thing and stays banned.

`SDL_DIALOG` is forced `ON` before `add_subdirectory(engine)`. The engine leaves SDL's dialog
subsystem off by default, and the first start needs a native file picker so a player can point at
their own ROM; the setting is forced because the engine's own non-`FORCE` default would otherwise
win.

## Build options

| Option | Default | Effect |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` | Set when neither it nor a multi-config generator supplies one, so an unconfigured build is optimized rather than an unoptimized debug build. `Debug` keeps symbols and skips the trimming below. |
| `BUILD_TESTING` | `ON` | Standard CTest option. Off skips the `tests/` subdirectory entirely. |

The port declares no options of its own yet.

`compile_commands.json` is always exported, for clangd and IDE include resolution.

## A game, not a command-line program

Both desktop platforms decide what an executable *is* from how it is built, and the default on each
is "a terminal program". Double-clicked, a bare Mach-O is handed to Terminal.app, and a
console-subsystem Windows binary is given a console window whether it wants one or not. Neither has
anything to do with the game's own output — they happen before `main` runs.

**macOS** builds an `.app`. That is what the Finder launches directly, what a signature and a
notarization ticket attach to, and what carries the identifier and version. The executable inside
keeps the artifact name, so the bundle is `UncannyPlumber.app/Contents/MacOS/UncannyPlumber`. The
`Info.plist` is generated from `cmake/UncannyPlumberBundleInfo.plist.in` and carries the identity,
both version strings, the minimum system version, the application category, and
`NSHighResolutionCapable`. That last one matters: the game draws at 160×144 and the engine scales
it, so without the Retina flag the window is drawn at 1× and then stretched — exactly the blur the
integer blit exists to avoid. No usage-description strings are present, because the game asks for
no protected resource; the one file-system prompt it can raise is the first-start ROM picker, which
is a user-initiated open panel and needs no entitlement.

The bundle identifier is `com.uncannyplumber.uncannyplumber`. **It is chosen once and never
changed** — the player's saves and extracted assets are found under this identity, and a
notarization ticket is bound to it. Changing it strands both.

**Windows** sets `WIN32_EXECUTABLE`, which links against the windows subsystem and is what stops
the console window from appearing. That normally also demands `WinMain`, but the engine defines
`SDL_MAIN_HANDLED` and calls `SDL_SetMainReady()` itself, so the game keeps a plain `main()`. Under
MSVC the entry point is named explicitly with `/ENTRY:mainCRTStartup` rather than pulling in SDL's
main shim to bridge the two.

## What the shipped binary carries

Release is the default build type, the linker drops what nothing reaches, and the symbol table
comes off. A `Debug` build keeps all of it. MSVC's Release link already does both, so the whole
block is skipped there.

| Platform | Unreferenced code | Symbols |
|---|---|---|
| macOS | `-Wl,-dead_strip` at the final link | `strip -x` |
| Linux | `-ffunction-sections -fdata-sections` on both targets, `-Wl,--gc-sections` at the link | `strip` |
| Windows (MSVC) | Handled by the Release link | Handled by the Release link |

Apple's `-x` keeps global symbols and drops the local ones, which leaves the engine's baked-routine
anchors reachable. Elsewhere the executable's whole symbol table goes.

The dead-strip flags are configuration-aware generator expressions, but the strip step is decided
at configure time from `CMAKE_BUILD_TYPE`. Under a multi-config generator that variable is empty,
so the strip is scheduled for every configuration; Ninja and Makefiles, which the project is
developed against, set it and behave as the table describes.

### Signing, last

On macOS the bundle is ad-hoc signed **after** the strip, as the final post-build step:

```
codesign --force --sign - <bundle>
codesign --verify --strict <bundle>
```

Apple silicon will not run an unsigned binary at all, so the linker ad-hoc signs every build. The
strip edits the binary afterwards, which invalidates that signature — and a bundle's signature has
to cover its `Info.plist` and seal its resources besides, which a linker signature over the
executable alone never does. Left that way, macOS calls the result damaged rather than unsigned and
refuses to open it: a broken signature is not something a player can wave past, where an absent one
is. Anything new that modifies the binary belongs **above** this step.

Ad-hoc means a signature with no identity behind it — enough to run, not enough for someone else's
machine to trust a download. Signing for distribution replaces this and belongs to the release.

## Warnings

`-Wall -Wextra -Wpedantic` on GCC and Clang; `/W4 /permissive-` plus `/Zc:__cplusplus` on MSVC,
which otherwise reports `__cplusplus` as `199711L` regardless of the standard in force.

Port sources build warning-free, and that is the standard to hold. The engine and its vendored
third-party trees produce their own warnings; those are not yours to fix from here.

## Layout

```
CMakeLists.txt        project metadata, standard, floors, flags, engine guard
cmake/                dependency declarations and the macOS bundle Info.plist template
src/
  main.cpp            entry point
  version.{h,cpp}     the port's version string, from the CMake project version
include/uncannyplumber/  public headers
tests/                GoogleTest cases
tools/                port-time tooling
assets/               canonical asset locations; contents gitignored
engine/               Retro++ submodule
docs/                 these pages, plus features/ and contracts/
```

## Where to change things

| To change | Edit |
|---|---|
| Compiler floors, warnings, C++ standard, build-type default | `CMakeLists.txt` |
| A fetched dependency's version | `cmake/Dependencies.cmake` |
| Targets, linkage, trimming, signing, the platform executable kinds | `src/CMakeLists.txt` |
| Bundle keys — category, minimum system, Retina flag, copyright | `cmake/UncannyPlumberBundleInfo.plist.in` |
| The version both the binary and the bundle report | `project(... VERSION ...)` in `CMakeLists.txt` |

## Logging

spdlog, used directly — there is no port-side logging wrapper, and the engine provides no logging
surface. Call `spdlog::info` / `warn` / `error` where you need them.
