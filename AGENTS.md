# StudioPatchbay OS9 Rework - Agent Instructions

## Project Purpose
StudioPatchbay is being rewritten from its original Swift/SwiftUI implementation to a cross-platform C++ application using the JUCE framework. The new version introduces a classic Mac OS 9 "Platinum" interface and runs on macOS and Windows.

## Branch Strategy & Source Control
- **`main` branch**: Contains the CURRENTLY PUBLISHED Swift/SwiftUI application. **DO NOT modify, merge into, or rebase `main`.** It must remain intact until the JUCE rewrite is fully complete and ready to replace it.
- **`os9-rework` branch**: The ACTIVE development branch for the JUCE/C++ rewrite. All work happens here. The entire rewrite lives inside the `JUCE/` subdirectory.
- **DO NOT create a separate repository** for the JUCE version. It will eventually replace the original version in this exact repository (`painfilter/StudioPatchbay`).

## Architecture & Workflow Rules
- **UI Framework**: JUCE (specifically standalone GUI app, not a plugin).
- **Look and feel**: a minimal Mac OS 9 "Platinum" look and feel (`StudioPatchbay::Design::PlatinumLookAndFeel`) in `JUCE/Source/Design/`. The repository builds on its own; it has no build or runtime dependency on anything outside it.
- **Parsing**: `juce::XmlDocument` is used for parsing Cubase configuration files (replacing Swift XML parsers).
- **File Watching**: Uses a `juce::Timer`-based polling mechanism for cross-platform file watching.
- **Components**:
  - `AppState`: Central state management, data loading, and event broadcasting.
  - `AppContentComponent`: the simulated desktop inside the native window: menu bar, the floating application window (`UI/AppWindow`), WindowShade with the desktop icons (`UI/DesktopShelf`), and the About/licence/message overlays.
  - `Workspace`: the window's content (Sidebar, ViewSwitcher, Views, Detail Panel, status line).
  - Texts: every visible string goes through `Text::tr ("key")`; keys live in `JUCE/Resources/Localization/StudioPatchbay.<lang>.json`. `common.<lang>.json` there is generated like `Source/Design/`.
- **Build System**: CMake with `FetchContent` for JUCE and Catch2. Build target is inside the `JUCE/` folder.

## Generated Files
The files in `JUCE/Source/Design/` and `JUCE/Resources/Localization/common.*.json` are generated. Do not edit them by hand: a
change there is overwritten the next time they are regenerated. If the look
and feel needs a change, describe it to the maintainer instead. Product code
(views, layout, the menu bar) lives in `JUCE/Source/UI/` and is edited normally.
