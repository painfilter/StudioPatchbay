# StudioPatchbay OS9 Rework - Project Status

## Current Status: IMPLEMENTATION (feature-complete for a first JUCE release, pending CI and manual tests on macOS and Windows)

The JUCE/C++ rewrite of StudioPatchbay with a classic Mac OS 9 "Platinum"
interface lives in `JUCE/`. It builds on macOS, Windows and Linux (Linux only
for tests; Cubase does not run there).

### Completed Work
- **Phase 0:** Project scaffold with CMake, `Main.cpp`, `MainWindow`, bundled fonts.
- **Phase 1:** Data model and parsers (`CubaseConfigParser`, `PlatformPaths`, `FileWatcher`).
- **Phase 2:** Chrome (`OS9MenuBar` with clock, desktop dot pattern, striped title bar).
- **Phase 3:** Sidebar & Navigation (`AppState`, `SidebarPanel`, `ViewSwitcher`).
- **Phase 4:** Cards View (`DeviceCardView`, `CardsView` with section headers).
- **Phase 5:** Patchbay Grid View (`PatchbayGridView`).
- **Phase 6:** Control Room (`ControlRoomView`) and Signal Flow (`SignalFlowView`).
- **Phase 7:**
  - `DeviceDetailPanel`, `AboutDialog` (licences), keyboard shortcuts (Cmd+1-4, Cmd+R, Cmd+Q).
  - File > Export as PDF / PNG of the active view's full content (`JUCE/Source/Export/`).
    PNG at 2x; PDF is a single page holding the same raster (1 logical pixel = 1 pt).
  - Windows support in `PlatformPaths`: `%APPDATA%\Steinberg\Cubase NN_64\`;
    macOS: `~/Library/Preferences/Cubase NN/`.
  - Reload re-reads the files and keeps the selected installation.
  - Theme and zoom are remembered (`AppSettings`, versioned settings file).
- **Look and feel:** `JUCE/Source/Design/` is generated; do not edit by hand.
- **CI:** `.github/workflows/ci.yml` builds and tests on Linux (tests only),
  macOS (universal app + tests) and Windows x64 (app + tests), and uploads
  unsigned app archives as workflow artifacts.

### Tests
Catch2, run by `ctest`: parsers, Cubase folder detection (both platform
spellings), app data directory, PDF writer (structure, xref offsets, pixel round-trip).

### Remaining Work / Next Tasks
1. Manual test on a real Cubase setup on macOS and on Windows (detection,
   live reload, export, dark/light, every zoom level).
2. Features of the Swift app not yet ported: converter/interface editor
   (physical devices), filtering and search.
3. Release packaging: macOS signing + notarisation (`packaging/release.sh` is
   for the Swift app), Windows code signing / installer.
4. Replace the Swift app on `main` once the above is done (with approval).
