# Studio Patchbay

A native macOS app that visualises your Cubase external devices (instruments, FX, sends)
as a live patchbay, reading directly from Cubase's `External Plugins.xml` preference file.

## Screenshots

**Cards** — every external device Cubase knows about, grouped by type, with its
channels, stereo/mono format and MIDI assignment.

![Cards view](docs/screenshots/cards.webp)

**Patchbay** — the whole channel space laid out per converter/interface, the OUT
row above the IN row, colour-coded by device type. Unused channels stay dimmed.

![Patchbay view](docs/screenshots/patchbay.webp)

**Detail panel** — click any card or channel cell to trace that device's whole
signal flow: out of the Cubase track, through the converter and interface, and
back in — plus its format, MIDI device and channel assignments.

![Detail panel with signal flow](docs/screenshots/signal-flow.webp)

**Converters & Interfaces** — map each converter to its channel range; the
physical ports are auto-detected from Cubase's `Port Setup.xml`.

![Converters and interfaces](docs/screenshots/converters.webp)

## Features

- **Auto-detects all Cubase versions** — scans `~/Library/Preferences/Cubase NN/`
  for any version ≥ 12. Works today with Cubase 15, and will automatically pick up
  Cubase 16 (and beyond) as soon as it is installed.
- **Live file watching** — uses `DispatchSource` (kqueue) to watch the XML file.
  When Cubase exits and writes its preferences, the app reloads within ~400ms
  automatically. A green indicator in the toolbar shows the watcher is active.
- **Two views:**
  - *Cards* — each device as a card grouped by type (Instruments / External FX / FX Sends),
    showing MADI channel numbers, stereo/mono format, MIDI device assignment.
  - *Patchbay* — the full MADI channel space as a grid. Instruments are blue,
    FX are orange, send-only routes are purple. Unused channels are dimmed.
- **Detail panel** — click any card or channel cell to see the full signal flow chain
  for that device, plus a warning if send and return channels differ.
- **Filtering & search** — filter by type, format (stereo/mono), or MIDI assignment.

## Requirements

- **Runs on macOS 15 (Sequoia) or later.** On macOS 26 (Tahoe) the toolbar uses
  the Liquid Glass APIs (`sharedBackgroundVisibility`, `ToolbarSpacer`); on
  earlier macOS it falls back to the standard toolbar automatically.
- **Builds with Xcode 26 or later** — the macOS 26 SDK is needed to compile the
  availability-guarded Liquid Glass code (it still runs on macOS 15+).
- Cubase 12 or later (any version that writes `External Plugins.xml`)

## Build & Run

1. Open `StudioPatchbay.xcodeproj` in Xcode
2. Select your Mac as the run destination
3. Press ⌘R

No signing setup needed for local development — Xcode will use automatic signing.

## Why no sandbox?

The app reads `~/Library/Preferences/Cubase NN/External Plugins.xml` directly.
The macOS sandbox would require the user to grant access via a file picker each launch,
which defeats the purpose of seamless live reloading. This app is not intended for
Mac App Store distribution. If you ever want to sandbox it, replace the direct file
read with a security-scoped bookmark persisted in `UserDefaults`.

## File location logic

`CubaseDetector.swift` — `detectCubaseInstallations()` scans
`~/Library/Preferences/` for directories matching `Cubase NN` where NN ≥ 12.
This means Cubase 16, 17, etc. will be picked up automatically with no code changes.
The app defaults to the highest version that has an `External Plugins.xml` present.

## How the file watcher works

`FileWatcher.swift` opens the file with `O_EVTONLY` and creates a
`DispatchSource.makeFileSystemObjectSource` watching for `.write` and `.rename` events.
Cubase writes atomically via rename, so both events are monitored. After an event,
the app waits 400ms (to let Cubase finish) then re-reads the file and re-parses.

## License

MIT — see [LICENSE](LICENSE). Free to use, modify and share. Contributions are
welcome; this is a casually maintained side project, so please be patient with
issues and PRs.

## Acknowledgments

StudioPatchbay was designed and built with [Claude Code](https://claude.com/claude-code).

## About

StudioPatchbay is open-source studio tooling from **Painfilter Software & Music
Production** — [painfilter.de](https://painfilter.de).

If you document your studio, you may also like **HardwareRecall**, a VST3/AU
plug-in that recalls the settings of your analog outboard gear inside your DAW
project. More at [painfilter.de/software](https://painfilter.de/software).
