# Features Changelog

Chronological log of feature status transitions, newest first. Entries are not edited after they
are written. `../FEATURES.md` holds current status; this file holds history.

**Format:** `<feature> <old status> → <new status>` with a one-line reason. Glyphs match
`../FEATURES.md`: ⬜ pending · 🟡 in progress · ✅ delivered · 🟫 dropped · ❌ rejected.

---

## 2026-08-28

- **Repository scaffolding and ignore rules** ⬜ → ✅. Repository initialized; ROM extensions and
  every extracted-content directory ignored tree-wide.
- **Build system** ⬜ → ✅. CMake 3.28+ / Ninja / C++20, Release by default with dead-strip and
  symbol strip; builds green on macOS against the engine.
- **Retro++ engine adoption** ⬜ → ✅. `engine/` submodule at `594774c`; SDL3 and SameBoy arrive
  transitively. The port declares no SDL3.
- **Test harness** ⬜ → ✅. GoogleTest wired through CTest; three-case smoke suite green.
- **Logging** ⬜ → ✅. spdlog, direct.
- Feature registry seeded; every game feature ⬜.
