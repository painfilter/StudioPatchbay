# Contributing

Thanks for looking. This is a side project maintained by one person, so replies
may take a while, but issues and pull requests are genuinely welcome.

## Reporting a parsing bug

This is the most useful kind of report, and the one hardest to guess at from a
description. The app reads files written by Cubase, and those files vary by
Cubase version, audio driver and interface. What was tested here is one studio's
setup, so your hardware very likely produces shapes this code has never seen.

Please include:

- Your Cubase version and macOS version
- Which audio interface and driver (ASIO or plain CoreAudio) you use
- What you expected to see, and what the app showed instead
- **A redacted snippet of the relevant file.** Usually
  `~/Library/Preferences/Cubase NN/External Plugins.xml`, or `Port Setup.xml`
  for anything about converters and channel numbers.

Device names in those files are yours to redact, and the parser does not care
what they say. What matters is the structure: the element nesting, the attribute
names, and the shape of the `Connections` and `SysName` values. Please keep those
intact, since replacing them with placeholders usually removes the exact detail
that explains the bug.

## Building

Open `StudioPatchbay.xcodeproj` in Xcode 26 or later and press ⌘R. The macOS 26
SDK is required to compile the availability-guarded Liquid Glass toolbar code,
which still runs on macOS 15.

## Tests

The parsers are covered by tests that do not need the app or a GUI:

```bash
swift test
```

If you fix a parsing bug, please add a test with a fixture that reproduces it.
The existing tests in `Tests/PatchbayCoreTests` build their XML inline with small
helpers, so a new case is usually a few lines. A parsing fix without a fixture
tends to regress the next time the format shifts.

CI runs `swift test` on every push. The app build also runs, but it is allowed to
fail, because hosted runners do not always carry the Xcode version the app needs.

## Code

- Keep the parsing layer free of SwiftUI and AppKit. It is deliberately pure
  Foundation so it stays testable, and `Package.swift` will fail to build if a UI
  import creeps in.
- Match the surrounding style rather than reformatting nearby code.
- Explain *why* in comments where the reason is not obvious from the code. The
  parsers document which real-world hardware produced which file format, and that
  context is worth more than a restatement of what the line does.

## Scope

The app deliberately does one thing: show what Cubase already knows about your
external devices. It is not a general studio database, and it does not store
routing of its own beyond the converter layout you configure. Features that would
turn it into a catalogue to maintain by hand are probably out of scope, but ask
before building something large, rather than after.
