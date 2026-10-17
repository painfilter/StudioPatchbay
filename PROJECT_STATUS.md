# StudioPatchbay OS9 Rework - Project Status

## Current Status: IMPLEMENTATION (functionally on par with the Swift app; manual tests on macOS and Windows pending)

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
  - `DeviceDetailPanel`, keyboard shortcuts (Cmd+1-4, Cmd+R, Cmd+Q).
  - File > Export as PDF / PNG of the active view's full content (`JUCE/Source/Export/`).
    PNG at 2x; PDF is a single page holding the same raster (1 logical pixel = 1 pt).
  - Windows support in `PlatformPaths`: `%APPDATA%\Steinberg\Cubase NN_64\`;
    macOS: `~/Library/Preferences/Cubase NN/`.
  - Reload re-reads the files and keeps the selected installation.
  - Theme and zoom are remembered (`AppSettings`, versioned settings file).
- **Shell (stage 1 of the 1:1 port):** the native window holds a simulated
  desktop (`AppContentComponent`): menu bar with product mark, clock and product
  name; the application window floats on the desktop (`UI/AppWindow`, content in
  `Workspace`). The collapse box shades the window onto the desktop with a
  220 ms ZoomRects outline; while collapsed, the desktop shows StudioPatchbay
  (bottom left, restores), the trash (bottom right, About), and in a grid the
  Cubase installations (click shows that version), PDF/PNG export, the watcher
  and the last exports (`UI/DesktopShelf`). File and View are off while collapsed.
  About, licences and messages are overlays. Options: Window Zoom, Language
  (System/English/Deutsch), Appearance (System/Light/Dark). Zoom keeps the
  logical desktop size and resizes the window; dragging the window edge keeps
  the zoom. Settings file v2 (appearance, language, window size, last Cubase
  version; v1 `darkMode` is migrated).
- **Converters & Interfaces (stage 2):** `Model/PhysicalDevice`, `Model/AudioPorts`,
  `Model/DeviceStore` read and write `physicalDevices.json` and `portLabels.json`
  in the Swift app's format and folder (both apps share one setup; old numeric
  ids and missing later keys load as in Swift). Ports are built from Port Setup
  by family with send/receive runs paired by position; labels use the
  `family@channel` key with the two older key forms as fallback. The window
  (`UI/Devices/`, Options menu or desktop icon) lists devices by type with a
  filter, edits with free start channels only and a conflict check, duplicates,
  deletes, removes all, exports and imports (review with the same conflict
  rule), shows and copies the save file path, and lists the detected ports with
  a Cubase version switch and optional interface names. Improvements over
  Swift: a new device starts on free channels; an unreadable devices file is
  set aside before the next save instead of being overwritten.
- **Views (stage 3), as in the Swift app:** sidebar with Cubase version (only
  installations with External Plugins.xml can be chosen; the last choice is
  remembered), filter (All, Instruments, External FX, Send Only, Stereo, Mono,
  Has MIDI), view and setup link; toolbar with Back/Forward, counts and search
  (name, MIDI device, channel number, physical device name; Return or the match
  count jumps to the next match in the patchbay); watch status, Reload and the
  content zoom 60-200 % (remembered). Cards in three sections; Patchbay by port,
  converter block (OUT above IN, physical I/O numbers, Cubase channel below),
  audio interfaces and Control Room, hover details, click to select, several
  devices on one channel cycle; Control Room cards with routes; the detail
  panel with signal flow (nodes jump to their channel) and details. Empty and
  error states as in Swift. Logic in `Model/Browsing`, `Model/PatchbayLayout`,
  `Model/SignalFlow` (tested). Improvements: long rows wrap instead of being
  cut, View menu and shortcuts for Back/Forward (Cmd+[ ]), Search (Cmd+F),
  zoom (Cmd+ +/-/0) and Converters & Interfaces (Cmd+,). The separate
  Signal Flow tab of the earlier JUCE draft is gone: the flow is in the detail
  panel, as in Swift.
- **Export (stage 4):** File > Export Patchbay as PDF/PNG and the desktop export
  programs write the patchbay as the Swift app does (everything, no search or
  selection, 1400 px wide at the current content zoom, a "Connected" table under
  each block, 2x raster); the current view can still be exported on its own.
  `StudioPatchbay --export <file.pdf|png>` does the same from a script.
- **Texts:** `JUCE/Resources/Localization/StudioPatchbay.<lang>.json` (product)
  and `common.<lang>.json` (generated, do not edit).
- **Look and feel:** `JUCE/Source/Design/` is generated; do not edit by hand.
- **CI:** `.github/workflows/ci.yml` builds and tests on Linux (tests only),
  macOS (universal app + tests) and Windows x64 (app + tests), and uploads
  unsigned app archives as workflow artifacts.

### Tests
Catch2, run by `ctest`: parsers, Cubase folder detection (both platform
spellings), app data directory, PDF writer (structure, xref offsets, pixel
round-trip), settings (v1 migration, round trip, newer files untouched),
texts (English and German have the same keys; every key the code uses exists),
physical devices (ownership per direction, range labels, conflicts, import
check, free start channels, reading a Swift-written file), ports (pairing,
label fallbacks), device store (first launch, labels, unreadable file),
search and filter, navigation history, patchbay blocks (asymmetric and
one-way devices, audio interfaces left out, overlapping ports), signal flow.

### Remaining Work / Next Tasks
1. Manual test on a real Cubase setup on macOS and on Windows (detection,
   live reload, the shared physicalDevices.json, views, export, language,
   appearance, window zoom and resizing, WindowShade).
2. Release packaging: macOS signing + notarisation (`packaging/release.sh` is
   for the Swift app), Windows code signing / installer.
3. Replace the Swift app on `main` once the above is done (with approval).
