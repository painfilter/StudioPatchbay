import Foundation

// MARK: - Detected Port model

/// One physical port family, auto-detected from Cubase's own Port Setup.xml rather
/// than hand-derived from documentation or verbal description. Cubase writes a
/// `SysName` for every channel in the form "<label> <local index or L/R> - <absolute
/// channel>" (e.g. "ADAT 1 - 195", "AES L - 193", "MADI 1 - 1"), independently for
/// send and recv. Grouping consecutive channels that share a label reconstructs
/// exactly the port layout that used to be hardcoded in `AudioPortLayout` — including
/// asymmetric send/recv offsets, which fall out automatically by comparing the two
/// directions instead of being reverse-engineered from device data by hand.
struct DetectedPort: Identifiable, Hashable {
    var id: String { "\(direction == .send ? "O" : "I")-\(family)-\(channelRange.lowerBound)" }
    let family: String              // e.g. "MADI", "ADAT", "AES", or a device name like "MOTU MicroBook"
    let direction: ChannelDirection
    let channelRange: ClosedRange<Int>
    /// True when this came from the fallback `Settings`-list parse, where Cubase's
    /// Port Setup.xml has no absolute channel number at all (unlike the aggregate-bus
    /// format) — the range is synthesized as a plausible sequential 1…N per device,
    /// not read from real Cubase data. Surfaced in the UI so it's never mistaken for
    /// an authoritative channel number the way the aggregate-format data is.
    var isApproximate: Bool = false
}

enum PortSetupParser {

    /// Parses Cubase's Port Setup.xml (found at the root of the Cubase prefs folder,
    /// alongside External Plugins.xml — not inside Presets/ like Control Room).
    ///
    /// Cubase writes one of two structurally different formats depending on the audio
    /// driver, discovered by testing against three real machines/interfaces:
    /// - A rich "PortDescriptors" list with absolute Cubase channel numbers, seen only
    ///   for a genuine ASIO-driver interface (confirmed: an RME setup combined into a
    ///   macOS Aggregate Device still produced this format).
    /// - A flatter "Settings" list with no absolute channel numbers at all, seen for
    ///   plain CoreAudio devices — Built-in Audio, a MOTU MicroBook (both standalone
    ///   and combined into a macOS Aggregate Device). A macOS Aggregate Device, it
    ///   turns out, is always presented as CoreAudio at the OS level regardless of
    ///   what real hardware feeds into it — so aggregation itself isn't what
    ///   determines the format, ASIO vs. CoreAudio is.
    static func parse(data: Data) throws -> [DetectedPort] {
        let xml = try XMLDocument(data: data, options: [])
        guard let root = xml.rootElement() else { return [] }

        let aggregateResult = parseAggregateBusList(root)
        if !aggregateResult.isEmpty { return aggregateResult }
        return parseSettingsListFallback(root)
    }

    /// The rich format: a "PortDescriptors" list with Type="audio" + SysName entries
    /// carrying real absolute Cubase channel numbers.
    private static func parseAggregateBusList(_ root: XMLElement) -> [DetectedPort] {
        guard let descriptorList = root.elements(forName: "list").first(where: {
            $0.attribute(forName: "name")?.stringValue == "PortDescriptors"
        }) else { return [] }

        // Step 1: collect every (direction, family, absoluteChannel) triple from the
        // aggregate bus entries. These are the ones with Type=="audio" and a SysName —
        // the OTHER entries in this same list (per-card raw driver ports, MIDI Remote
        // control surfaces, etc.) don't have that shape and are skipped naturally.
        struct Entry { let direction: ChannelDirection; let family: String; let channel: Int }
        var entries: [Entry] = []

        for item in descriptorList.elements(forName: "item") {
            guard let idValue = string(in: item, elementName: "string", attrName: "ID"),
                  let sysName = string(in: item, elementName: "string", attrName: "SysName")
            else { continue }

            let direction: ChannelDirection = idValue.hasPrefix("O|") ? .send : .recv

            // Same "...HDSPe Device N" convention used by External Plugins.xml and
            // Control Room's Connections lists — the channel number is the ID's
            // trailing integer, which is more robust than parsing it back out of the
            // human-readable SysName text.
            guard let channel = trailingInt(idValue) else { continue }
            // A handful of non-audio entries (MIDI device ports, etc.) coincidentally
            // share the same Type="audio"/SysName shape but use huge internal ID
            // numbers that aren't real Cubase channel numbers — reject anything past
            // a generous real-world ceiling rather than let them pollute the port list.
            guard channel > 0, channel <= 2048 else { continue }

            // SysName format: "<label> - <channel>", e.g. "ADAT 1 - 195", "AES L - 193".
            guard let labelPart = sysName.components(separatedBy: " - ").first else { continue }
            let family = stripTrailingIndex(labelPart)
            guard !family.isEmpty else { continue }

            entries.append(Entry(direction: direction, family: family, channel: channel))
        }

        // Step 2: group by (direction, family), then split into contiguous runs.
        // The same family label can legitimately appear more than once non-
        // contiguously (e.g. "AES" shows up both on the MADI FX at ch 193–194 and
        // again on the RayDAT at ch 227–228/229–230) — a gap in channel numbers is
        // what separates them into distinct ports.
        var grouped: [String: [Int]] = [:]
        for e in entries {
            grouped[groupKey(e.direction, e.family), default: []].append(e.channel)
        }

        var ports: [DetectedPort] = []
        for (key, channels) in grouped {
            let (direction, family) = splitGroupKey(key)
            for run in contiguousRuns(in: channels.sorted()) {
                ports.append(DetectedPort(family: family, direction: direction, channelRange: run))
            }
        }

        return ports.sorted { $0.channelRange.lowerBound < $1.channelRange.lowerBound }
    }

    /// The flat fallback format: a "Settings" list of raw per-port entries
    /// ("O|GUID|MOTU MicroBook 1", "I|GUID|Built-in Audio L", etc.) with no absolute
    /// Cubase channel number anywhere. Groups by device name (same trailing-index-
    /// stripping logic as the rich format handles both "Label N" and "Label L"/"Label
    /// R" shapes already) and assigns sequential 1…N numbering per device+direction,
    /// in file order — the best available approximation, not real Cubase data, hence
    /// `isApproximate: true`. Explicitly skips "MIDI System" entries, which use this
    /// same list but aren't audio ports at all.
    private static func parseSettingsListFallback(_ root: XMLElement) -> [DetectedPort] {
        guard let settingsList = root.elements(forName: "list").first(where: {
            $0.attribute(forName: "name")?.stringValue == "Settings"
        }) else { return [] }

        var order: [String] = []
        var counts: [String: Int] = [:]
        var directionByKey: [String: ChannelDirection] = [:]
        var familyByKey: [String: String] = [:]

        for item in settingsList.elements(forName: "item") {
            guard let idValue = string(in: item, elementName: "string", attrName: "ID") else { continue }
            let direction: ChannelDirection = idValue.hasPrefix("O|") ? .send : .recv

            let parts = idValue.components(separatedBy: "|")
            guard parts.count >= 3, parts[1] != "MIDI System" else { continue }
            let family = stripTrailingIndex(parts[2])
            guard !family.isEmpty else { continue }

            let key = groupKey(direction, family)
            if counts[key] == nil {
                order.append(key)
                directionByKey[key] = direction
                familyByKey[key] = family
            }
            counts[key, default: 0] += 1
        }

        return order.map { key in
            DetectedPort(family: familyByKey[key]!, direction: directionByKey[key]!,
                         channelRange: 1...counts[key]!, isApproximate: true)
        }
    }

    // MARK: - Helpers

    private static func string(in item: XMLElement, elementName: String, attrName: String) -> String? {
        item.elements(forName: elementName)
            .first { $0.attribute(forName: "name")?.stringValue == attrName }
            .flatMap { $0.attribute(forName: "value")?.stringValue }
    }

    private static func trailingInt(_ s: String) -> Int? {
        let digits = s.reversed().prefix { $0.isNumber }
        guard !digits.isEmpty else { return nil }
        return Int(String(digits.reversed()))
    }

    /// Strips a trailing local index ("MADI 1" → "MADI") or L/R role ("AES L" → "AES")
    /// to get the port family name shared across all channels of that physical port.
    private static func stripTrailingIndex(_ label: String) -> String {
        var s = label.trimmingCharacters(in: .whitespaces)
        if s.hasSuffix(" L") || s.hasSuffix(" R") {
            s = String(s.dropLast(2))
        } else if let lastSpace = s.lastIndex(of: " "),
                  Int(s[s.index(after: lastSpace)...]) != nil {
            s = String(s[..<lastSpace])
        }
        return s.trimmingCharacters(in: .whitespaces)
    }

    private static func groupKey(_ direction: ChannelDirection, _ family: String) -> String {
        "\(direction == .send ? "O" : "I")::\(family)"
    }
    private static func splitGroupKey(_ key: String) -> (ChannelDirection, String) {
        let parts = key.components(separatedBy: "::")
        return (parts[0] == "O" ? .send : .recv, parts.count > 1 ? parts[1] : "")
    }

    private static func contiguousRuns(in sortedChannels: [Int]) -> [ClosedRange<Int>] {
        guard !sortedChannels.isEmpty else { return [] }
        var runs: [ClosedRange<Int>] = []
        var start = sortedChannels[0]
        var prev = sortedChannels[0]
        for ch in sortedChannels.dropFirst() {
            if ch != prev + 1 {
                runs.append(start...prev)
                start = ch
            }
            prev = ch
        }
        runs.append(start...prev)
        return runs
    }
}
