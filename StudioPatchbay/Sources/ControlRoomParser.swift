import Foundation

// MARK: - Control Room model

/// One Control Room monitoring bus (main Control Room, a Studio send, Phones, or a
/// Speaker Out), with the physical hardware output channels it's actually wired to.
struct ControlRoomBus: Identifiable, Hashable {
    enum Kind {
        case studio    // "Studio" sends, e.g. a headphone amp fed from Control Room
        case phones    // Direct Phones output
        case speaker   // Speaker Out (main monitors)

        var icon: String {
            switch self {
            case .studio:  return "star.fill"
            case .phones:  return "headphones"
            case .speaker: return "hifispeaker.fill"
            }
        }
        var label: String {
            switch self {
            case .studio:  return "Studio"
            case .phones:  return "Phones"
            case .speaker: return "Speaker Out"
            }
        }
    }

    let id: Int
    let name: String       // display name from Cubase, e.g. "Phones Amp (209+210)"
    let kind: Kind
    let channels: [Int]    // physical output channels feeding hardware, e.g. [209, 210]
}

enum ControlRoomParser {

    /// Parses Cubase's Control Room preset XML
    /// (`Presets/ControlRoom/Default.xml` under the Cubase prefs folder).
    /// Returns an empty array (not an error) if the file doesn't exist or has no
    /// wired buses yet — Control Room is optional, unlike External Plugins.xml.
    static func parse(data: Data) throws -> [ControlRoomBus] {
        let xml = try XMLDocument(data: data, options: [])
        guard let root = xml.rootElement() else { return [] }

        guard let presetMember = root.elements(forName: "member").first(where: {
            $0.attribute(forName: "name")?.stringValue == "Preset"
        }) else { return [] }

        guard let channelList = presetMember.elements(forName: "list").first(where: {
            $0.attribute(forName: "name")?.stringValue == "Channel"
        }) else { return [] }

        var buses: [ControlRoomBus] = []
        for (index, item) in channelList.elements(forName: "item").enumerated() {
            if let bus = parseChannel(item, id: index) {
                buses.append(bus)
            }
        }
        return buses
    }

    private static func parseChannel(_ item: XMLElement, id: Int) -> ControlRoomBus? {
        guard let name = nameString(in: item), !name.isEmpty else { return nil }

        // The main "Control Room" bus itself has no OutputBus (it only has an
        // OwnInputBus — it's the internal summing bus, not wired to hardware), so it's
        // naturally skipped here rather than needing an explicit type check for it.
        guard let outputBus = item.elements(forName: "member").first(where: {
            $0.attribute(forName: "name")?.stringValue == "OutputBus"
        }) else { return nil }

        let ch = connectionChannels(in: outputBus)
        guard !ch.isEmpty else { return nil }

        let kind: ControlRoomBus.Kind
        switch int(in: item, named: "Type") {
        case 15:      kind = .phones
        case 16:      kind = .speaker
        default:      kind = .studio   // 12 = Studio send (e.g. a headphone amp), or unrecognized
        }

        return ControlRoomBus(id: id, name: name, kind: kind, channels: ch)
    }

    private static func nameString(in item: XMLElement) -> String? {
        item.elements(forName: "member")
            .first { $0.attribute(forName: "name")?.stringValue == "Name" }
            .flatMap { nameMember in
                nameMember.elements(forName: "string")
                    .first { $0.attribute(forName: "name")?.stringValue == "String" }
            }
            .flatMap { $0.attribute(forName: "value")?.stringValue }
    }

    // Same "O|GUID|HDSPe Device NNN" connection format as External Plugins.xml.
    private static func connectionChannels(in outputBus: XMLElement) -> [Int] {
        var result: [Int] = []
        for connList in outputBus.elements(forName: "list") where
            connList.attribute(forName: "name")?.stringValue == "Connections" {
            for connItem in connList.elements(forName: "item") {
                guard let val = connItem.attribute(forName: "value")?.stringValue,
                      val.contains("|") else { continue }
                let last = val.components(separatedBy: "|").last ?? ""
                let chStr = last.replacingOccurrences(of: "HDSPe Device ", with: "")
                    .trimmingCharacters(in: .whitespaces)
                if let ch = Int(chStr) { result.append(ch) }
            }
        }
        return result
    }

    private static func int(in element: XMLElement, named attrName: String) -> Int? {
        element.elements(forName: "int")
            .first { $0.attribute(forName: "name")?.stringValue == attrName }
            .flatMap { $0.attribute(forName: "value")?.stringValue }
            .flatMap { Int($0) }
    }
}

extension Array where Element == ControlRoomBus {
    /// Every physical output channel used by any Control Room bus, mapped to the
    /// bus(es) using it — for overlaying onto the Patchbay's OUT rows.
    var channelMap: [Int: [ControlRoomBus]] {
        var map: [Int: [ControlRoomBus]] = [:]
        for bus in self {
            for ch in bus.channels {
                map[ch, default: []].append(bus)
            }
        }
        return map
    }
}
