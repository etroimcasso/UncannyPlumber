# Engine documentation

Guide for working with Uncanny Plumber's C++ surfaces — building it, running it, changing how it
behaves, and building on top of what is already there. Each area has its own page; the index is
below.

These pages describe the surface as it exists: what a type holds, what a function returns, what it
throws, where the backing data lives, and what to edit to change behavior. They are written for
someone modifying or extending Uncanny Plumber, not for someone checking it against the original
game — behavioral specifications derived from the Game Boy version live separately in
[`../contracts/`](../contracts/), and the design reasoning behind a given feature lives in
[`../features/`](../features/).

Uncanny Plumber is a native reimplementation, not an emulator. It runs as ordinary C++ on top of
the [Retro++](https://github.com/RetroPlusPlus/Engine) engine, which supplies the platform layer,
run loop, renderer, audio, and the virtual machine that hosts the handful of routines that need
one. Where a page says "the engine", it means Retro++; where it says "the port" or "Uncanny
Plumber", it means the code in this repository.

## Pages

| Page | Covers |
|---|---|
| [build.md](build.md) | Requirements and the first build; the three targets and how they fit together; the engine submodule and the fetched dependencies; build options; what the desktop build produces on each platform; how the shipped binary is trimmed and signed; warnings; the source layout. |

A page is added for each area as that area lands, so this index grows with the port. The game's
graphics, audio, level and text data are not in this repository and never will be — they come out
of a player's own ROM. See [`../features/asset-acquisition.md`](../features/asset-acquisition.md)
for how that works and what a build needs before it can run.
