# Changelog

Notable changes to StudioPatchbay. Dates are release dates.

## Unreleased

## 1.1 - 2026-07-23

Nothing about how the app reads Cubase changed in this release. It is tests,
wording and typography.

### Added
- Tests for the parsing layer (41 cases covering `ExternalPluginsParser`,
  `PortSetupParser` and `ControlRoomParser`), runnable with `swift test` without
  building the app. The parsers read a format Steinberg controls, so this is
  regression cover for the part most likely to break without anyone touching
  this repository.
- CI on every push, running the parser tests and building the app.
- `CONTRIBUTING.md` and issue templates, aimed at getting a redacted XML snippet
  with parsing reports, since that is what makes them fixable.

### Changed
- Section labels now share one treatment across the whole app: small, uppercase
  and letterspaced, reading as annotation on a technical drawing rather than as
  headlines competing with the data. This is the lettering idiom used by
  HardwareRecall, the other tool in the same family.
- Wording throughout the interface reads more plainly.
- An unknown device format now reads "Unknown" instead of a bare dash.

## 1.0 - 2026-07-22

First public release. Requires macOS 15 (Sequoia) or later and Cubase 12 or later.

- Reads external instruments, FX and sends directly from Cubase's
  `External Plugins.xml`, with no manual entry.
- Detects every installed Cubase version from 12 upwards and defaults to the
  highest one that has written a preferences file.
- Live file watching, reloading roughly 400ms after Cubase writes its
  preferences.
- Cards view and Patchbay grid view, colour-coded by device type.
- Detail panel tracing the full signal flow for a device, including a warning
  when send and return channels differ.
- Converter and interface mapping, with physical ports auto-detected from
  `Port Setup.xml`.
- Filtering and search by type, format and MIDI assignment.
- Export the patchbay as PDF or image.
