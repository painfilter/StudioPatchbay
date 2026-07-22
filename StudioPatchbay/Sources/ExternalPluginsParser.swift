import Foundation

// MARK: - Physical device model

enum DeviceDirectionality: String, Codable, CaseIterable {
    case bidirectional
    case sendOnly
    case recvOnly

    var displayName: String {
        switch self {
        case .bidirectional: return "Both directions (OUT + IN)"
        case .sendOnly:       return "Output only (OUT)"
        case .recvOnly:       return "Input only (IN)"
        }
    }
}

/// What role this entry actually plays, independent of its ConnectorType (the
/// digital protocol it uses). A converter (Ferrofish, ADI-648, etc.) only exists
/// between the RME's digital port and external gear — it has no connection to the
/// computer itself. A USB/Thunderbolt entry might instead BE a self-contained audio
/// interface (an RME, Universal Audio, or MOTU box with its own analog I/O) sitting
/// directly between the computer and the outside world. Those are different roles
/// even though both can use e.g. a "USB" ConnectorType, hence a separate field.
enum DeviceType: String, Codable, CaseIterable {
    case converter
    case audioInterface
    case other

    var displayName: String {
        switch self {
        case .converter:     return "Converter"
        case .audioInterface: return "Audio Interface"
        case .other:          return "Other"
        }
    }
    var icon: String {
        switch self {
        case .converter:      return "cable.connector"
        case .audioInterface: return "cpu"
        case .other:          return "questionmark.circle"
        }
    }
}

/// A physical AD/DA converter, standalone audio interface, or other hardware device
/// sitting between the RME and external gear (or, for an Audio Interface, connected
/// directly to the computer). User-editable via Settings — persisted as JSON, seeded
/// from `PhysicalDeviceLayout.defaultSeed` on first launch.
///
/// Named "PhysicalDevice" rather than the original "Converter" — the concept grew
/// well beyond converters specifically (audio interfaces, "other" hardware) and kept
/// that name only because it's what the model was first built as. The persisted
/// file on disk was renamed too (physicalDevices.json, from the legacy
/// converters.json), safe to do only because there was a single real user handling
/// the on-disk migration themselves — not something to repeat casually once there's
/// a wider user base with existing data riding on a given file name.
struct PhysicalDevice: Identifiable, Hashable, Codable {
    /// UUID rather than a sequential Int — this is what actually closes off the bug
    /// class we hit twice: an Int id regenerated via "max existing + 1" is only
    /// unique WITHIN whatever array it's currently in, so an imported device (with
    /// IDs from a completely different array, at a completely different time) could
    /// easily coincide with an existing one's ID by pure chance. A UUID can't
    /// coincidentally collide across import/export/duplicate in any realistic sense.
    let id: UUID
    var name: String
    var shortName: String
    var portColor: ConnectorType
    /// Converter, self-contained audio interface, or other — see DeviceType.
    var deviceType: DeviceType = .converter
    var physicalChannelCount: Int
    var sendChannelStart: Int            // first MADI/ADAT ch for the send (Cubase→hardware) direction
    var recvChannelStart: Int            // first MADI/ADAT ch for the recv (hardware→Cubase) direction
    /// Whether this device actually has both directions. Some hardware is
    /// genuinely one-way — a monitor system has no input path at all, a mic preamp
    /// might have no analog output — so forcing every device to claim both a send
    /// and a recv range misattributes whatever's on the direction it doesn't actually
    /// have. When a physical connector's OUT and IN sides go to a device on one
    /// side and something else entirely on the other, model that as this device
    /// being one-directional here, and let the other side show as an unattributed
    /// direct channel (no device) — which is correct, since a digital pass-through
    /// with no conversion happening doesn't need a converter at all.
    var directionality: DeviceDirectionality = .bidirectional
    /// Overrides the family-inherited port color with a user-chosen one, stored as
    /// "#RRGGBB". Nil falls back to `portColor`'s default.
    var customColorHex: String? = nil

    private enum CodingKeys: String, CodingKey {
        case id, name, shortName, portColor, deviceType, physicalChannelCount
        case sendChannelStart, recvChannelStart, directionality, customColorHex
    }

    init(id: UUID = UUID(), name: String, shortName: String, portColor: ConnectorType,
         deviceType: DeviceType = .converter, physicalChannelCount: Int,
         sendChannelStart: Int, recvChannelStart: Int,
         directionality: DeviceDirectionality = .bidirectional,
         customColorHex: String? = nil) {
        self.id = id
        self.name = name
        self.shortName = shortName
        self.portColor = portColor
        self.deviceType = deviceType
        self.physicalChannelCount = physicalChannelCount
        self.sendChannelStart = sendChannelStart
        self.recvChannelStart = recvChannelStart
        self.directionality = directionality
        self.customColorHex = customColorHex
    }

    // Custom Codable: Swift's synthesized Decodable does NOT fall back to a
    // property's default value when a key is missing — it only does that for
    // Optional properties. `directionality` and `deviceType` were added after this
    // feature first shipped, so a converters.json saved before either existed would
    // otherwise fail to decode entirely, silently discarding real edits back to
    // PhysicalDeviceLayout.defaultSeed. decodeIfPresent with an explicit fallback
    // keeps old files loading correctly as the schema grows.
    //
    // `id` also migrates gracefully: it used to be a plain Int. A file saved before
    // the UUID switch has a JSON number here instead of a UUID string — decode
    // whichever is present, and if it's the old Int form, replace it with a fresh
    // UUID rather than fail. That's safe: id is purely internal bookkeeping, never
    // shown to the user, so losing the OLD specific value costs nothing.
    init(from decoder: Decoder) throws {
        let c = try decoder.container(keyedBy: CodingKeys.self)
        if let uuidString = try? c.decode(String.self, forKey: .id), let uuid = UUID(uuidString: uuidString) {
            id = uuid
        } else {
            id = UUID()
        }
        name = try c.decode(String.self, forKey: .name)
        shortName = try c.decode(String.self, forKey: .shortName)
        portColor = try c.decode(ConnectorType.self, forKey: .portColor)
        deviceType = try c.decodeIfPresent(DeviceType.self, forKey: .deviceType) ?? .converter
        physicalChannelCount = try c.decode(Int.self, forKey: .physicalChannelCount)
        sendChannelStart = try c.decode(Int.self, forKey: .sendChannelStart)
        recvChannelStart = try c.decode(Int.self, forKey: .recvChannelStart)
        directionality = try c.decodeIfPresent(DeviceDirectionality.self, forKey: .directionality) ?? .bidirectional
        customColorHex = try c.decodeIfPresent(String.self, forKey: .customColorHex)
    }

    func encode(to encoder: Encoder) throws {
        var c = encoder.container(keyedBy: CodingKeys.self)
        try c.encode(id.uuidString, forKey: .id)
        try c.encode(name, forKey: .name)
        try c.encode(shortName, forKey: .shortName)
        try c.encode(portColor, forKey: .portColor)
        try c.encode(deviceType, forKey: .deviceType)
        try c.encode(physicalChannelCount, forKey: .physicalChannelCount)
        try c.encode(sendChannelStart, forKey: .sendChannelStart)
        try c.encode(recvChannelStart, forKey: .recvChannelStart)
        try c.encode(directionality, forKey: .directionality)
        try c.encodeIfPresent(customColorHex, forKey: .customColorHex)
    }

    func supports(_ direction: ChannelDirection) -> Bool {
        switch (direction, directionality) {
        case (.send, .recvOnly), (.recv, .sendOnly): return false
        default: return true
        }
    }

    /// Union of the send and recv ranges this device actually has — always
    /// derived, never stored, so it can never drift out of sync with the actual
    /// send/recv channels (a real bug we hit by hand-deriving this before it was
    /// computed). Only includes the direction(s) this device actually supports.
    var channelRange: ClosedRange<Int> {
        switch directionality {
        case .bidirectional:
            let s = range(for: .send), r = range(for: .recv)
            return min(s.lowerBound, r.lowerBound)...max(s.upperBound, r.upperBound)
        case .sendOnly: return range(for: .send)
        case .recvOnly: return range(for: .recv)
        }
    }

    /// The actual channel span for one direction. This — not `channelRange` — is the
    /// correct way to test ownership. Cubase numbers its send and receive busses
    /// independently, so two different devices can legitimately share a raw channel
    /// number as long as it's for different directions (e.g. one device's send
    /// channel can equal a neighboring device's recv channel).
    func range(for direction: ChannelDirection) -> ClosedRange<Int> {
        let start = direction == .send ? sendChannelStart : recvChannelStart
        return start...(start + physicalChannelCount - 1)
    }

    func contains(_ ch: Int, direction: ChannelDirection) -> Bool {
        supports(direction) && range(for: direction).contains(ch)
    }

    // Direction-agnostic — only safe for the union channelRange (header display).
    // Prefer contains(_:direction:) for anything that decides channel ownership.
    func contains(_ ch: Int) -> Bool { channelRange.contains(ch) }

    /// Physical I/O number (1-based) for a given channel, taking direction into account.
    func physicalIO(for channel: Int, direction: ChannelDirection) -> Int? {
        guard contains(channel, direction: direction) else { return nil }
        let start = direction == .send ? sendChannelStart : recvChannelStart
        let n = channel - start + 1
        guard n >= 1, n <= physicalChannelCount else { return nil }
        return n
    }

    /// Compact physical I/O label for a set of channels in one direction, e.g. "I/O 1–2"
    func physicalRangeLabel(for channels: [Int], direction: ChannelDirection) -> String? {
        let ios = channels.compactMap { physicalIO(for: $0, direction: direction) }.sorted()
        guard !ios.isEmpty else { return nil }
        if ios.count == 1 { return "I/O \(ios[0])" }
        if ios == Array(ios.min()!...ios.max()!) { return "I/O \(ios.first!)–\(ios.last!)" }
        return "I/O " + ios.map(String.init).joined(separator: "+")
    }

    // Legacy — used in device header strip (no direction context)
    func physicalIO(for channel: Int) -> Int? {
        physicalIO(for: channel, direction: .recv)
    }
    func physicalRangeLabel(for channels: [Int]) -> String? {
        physicalRangeLabel(for: channels, direction: .recv)
    }
}

enum PhysicalDeviceLayout {
    /// The built-in fallback for fresh installs — deliberately empty. A person's real
    /// device setup should live only in their own persisted store (and whatever
    /// file they choose to export), never baked into the shipped app as someone
    /// else's studio wiring.
    static let defaultSeed: [PhysicalDevice] = []

    /// The live, editable set — populated from the persisted store at launch (see
    /// `AppState.loadPhysicalDevices`), falling back to `defaultSeed` if nothing's
    /// persisted yet. Settings edits write here immediately so every view (which all
    /// read this fresh on each render) reflects changes right away.
    static var devices: [PhysicalDevice] = defaultSeed

    static func device(for ch: Int) -> PhysicalDevice? {
        devices.first { $0.contains(ch) }
    }

    /// Direction-aware lookup — the correct way to resolve channel ownership for
    /// grouping/highlighting/physical-I/O purposes. A raw Cubase channel number can
    /// belong to different devices depending on whether it's being read as a send
    /// or a recv channel, since Cubase numbers those independently.
    static func device(for ch: Int, direction: ChannelDirection) -> PhysicalDevice? {
        devices.first { $0.contains(ch, direction: direction) }
    }

    /// Same direction-aware lookup, but excluding a given DeviceType — used to keep
    /// Audio Interface devices (standalone USB/Thunderbolt devices with no
    /// relationship to the RME's own port map) out of the RME-topology Patchbay
    /// sections, since they get their own dedicated section instead.
    static func device(for ch: Int, direction: ChannelDirection, excluding excludedType: DeviceType) -> PhysicalDevice? {
        devices.first { $0.contains(ch, direction: direction) && $0.deviceType != excludedType }
    }

    /// True if any device of the given type claims this channel in either
    /// direction — used to exclude such channels from unrelated sections entirely,
    /// not even as an unlabeled "Unknown" fallback block.
    static func isChannelOwned(_ ch: Int, byDeviceType type: DeviceType) -> Bool {
        devices.contains { dev in
            dev.deviceType == type && (dev.contains(ch, direction: .send) || dev.contains(ch, direction: .recv))
        }
    }
}

// MARK: - Subdevice / Port model

// MARK: - Connector type

/// What kind of physical connector a converter or port actually has. Decoupled from
/// Cubase's channel *numbering* — a converter's connector type never changes based on
/// which channel numbers Cubase happens to assign it, only its color/categorization.
enum ConnectorType: String, Codable, CaseIterable {
    case madi
    case adat
    case aes
    case spdif
    case dante
    case usb
    case thunderbolt
    case analogLine   // instrument/line-level jacks
    case analogMic    // XLR mic inputs
    case other

    var displayName: String {
        switch self {
        case .madi:        return "MADI"
        case .adat:        return "ADAT"
        case .aes:         return "AES/EBU"
        case .spdif:       return "S/PDIF"
        case .dante:       return "Dante"
        case .usb:         return "USB"
        case .thunderbolt: return "Thunderbolt"
        case .analogLine:  return "Analog (Line/Instrument)"
        case .analogMic:   return "Analog (Mic/XLR)"
        case .other:       return "Other"
        }
    }

    /// Protocols relevant to a converter's connector type. This includes USB and
    /// Thunderbolt alongside the "plugged into the main card" protocols (MADI/ADAT/
    /// AES/S/PDIF/Dante), since a "converter" here can also be a self-contained audio
    /// interface (RME, Universal Audio, MOTU, etc.) connected straight to the computer
    /// rather than a box plugged into another interface's digital ports.
    static var digitalCases: [ConnectorType] { [.madi, .adat, .aes, .spdif, .dante, .usb, .thunderbolt, .other] }
}

// MARK: - Subdevice / Port model

struct AudioPort: Identifiable, Hashable {
    let id: Int
    let name: String
    let shortName: String
    let channelRange: ClosedRange<Int>
    let color: ConnectorType
    let rmeInterface: String   // physical RME device name

    func contains(_ channel: Int) -> Bool { channelRange.contains(channel) }
}

enum AudioPortLayout {
    /// Populated from Port Setup.xml via `updatePorts(from:)` — see `PortSetupParser`.
    /// Starts empty; if a reload ever fails or returns nothing (file missing, parse
    /// error), the previous good set is left in place rather than clearing to empty,
    /// same tolerance pattern used for Control Room's optional data.
    private(set) static var ports: [AudioPort] = []

    static func port(for channel: Int) -> AudioPort? {
        ports.first { $0.contains(channel) }
    }

    /// Rebuilds `ports` from auto-detected port families. Each family's send and recv
    /// runs are paired up by position (sorted by starting channel) rather than merged
    /// by exact channel equality, since asymmetric interfaces number the same physical
    /// port differently per direction (e.g. RayDAT's ADAT: recv starts ch 195, send ch
    /// 197) — positional pairing still correctly keeps genuinely separate physical
    /// ports apart (e.g. the MADI FX's AES pair vs. the RayDAT's AES pair, both family
    /// "AES" but at very different channel numbers).
    ///
    /// Deliberately no fixed-size chunking (e.g. old hardcoded 64-channel MADI ports):
    /// MADI's real channel count varies with sample rate/frame format, and other
    /// protocols (ADAT, Dante, etc.) have entirely different physical groupings that
    /// Cubase doesn't expose anyway. Physical devices (user-defined) are what actually
    /// subdivide a family's range into real physical connectors.
    static func updatePorts(from detected: [DetectedPort]) {
        guard !detected.isEmpty else { return }

        var byFamily: [String: (send: [ClosedRange<Int>], recv: [ClosedRange<Int>])] = [:]
        for port in detected {
            if port.direction == .send {
                byFamily[port.family, default: ([], [])].send.append(port.channelRange)
            } else {
                byFamily[port.family, default: ([], [])].recv.append(port.channelRange)
            }
        }

        var result: [AudioPort] = []
        var nextID = 0
        for (family, ranges) in byFamily.sorted(by: { $0.key < $1.key }) {
            let sendRuns = ranges.send.sorted { $0.lowerBound < $1.lowerBound }
            let recvRuns = ranges.recv.sorted { $0.lowerBound < $1.lowerBound }
            let instanceCount = max(sendRuns.count, recvRuns.count)

            for i in 0..<instanceCount {
                let s = i < sendRuns.count ? sendRuns[i] : nil
                let r = i < recvRuns.count ? recvRuns[i] : nil
                guard let lower = [s?.lowerBound, r?.lowerBound].compactMap({ $0 }).min(),
                      let upper = [s?.upperBound, r?.upperBound].compactMap({ $0 }).max() else { continue }

                let suffix = instanceCount > 1 ? " \(i + 1)" : ""
                result.append(AudioPort(
                    id: nextID,
                    name: "\(family)\(suffix)",
                    shortName: "\(family)\(suffix)",
                    channelRange: lower...upper,
                    color: colorForFamily(family),
                    rmeInterface: "\(family) Interface"
                ))
                nextID += 1
            }
        }

        ports = result.sorted { $0.channelRange.lowerBound < $1.channelRange.lowerBound }
    }

    /// Applies user-supplied physical-interface labels (Settings > Detected Physical
    /// Ports), keyed by each port's own `name` (e.g. "MADI", "ADAT", "AES 1") — stable
    /// across reloads since it's derived purely from the auto-detected family and
    /// instance position, not anything that changes between rebuilds. A missing or
    /// blank override just leaves the auto-generated "<family> Interface" default.
    /// Stable key for a port's label override — combines the port's family name
    /// (with any positional instance suffix stripped, e.g. "AES 2" → "AES") with its
    /// starting channel number.
    ///
    /// Channel number alone isn't unique: two genuinely different ports can share a
    /// starting channel (Phones sits entirely inside ADAT's range, both starting at
    /// 195 — the same overlap identified early in this project). Family name alone
    /// isn't stable: a name like "AES 2" is positional — adding a new AES port
    /// earlier in the channel range shifts every later instance's suffix, silently
    /// reattaching a saved label to the wrong physical port. Combining both fixes
    /// each other's gap: the channel number ties the key to real hardware instead of
    /// detection order, and the family name disambiguates same-start-channel ports
    /// the number alone can't tell apart.
    static func stableLabelKey(for port: AudioPort) -> String {
        let family = port.name.replacingOccurrences(of: #"\s+\d+$"#, with: "", options: .regularExpression)
        return "\(family)@\(port.channelRange.lowerBound)"
    }

    static func applyLabelOverrides(_ overrides: [String: String]) {
        ports = ports.map { port in
            // Three-tier fallback so labels saved under either of the two earlier
            // (each individually flawed) key schemes keep working until re-saved:
            // family+channel (current) → channel-only (briefly used, collided when
            // ports shared a start channel) → plain name (original, drifted when
            // ports got added/removed elsewhere).
            let label = overrides[stableLabelKey(for: port)]
                ?? overrides["ch\(port.channelRange.lowerBound)"]
                ?? overrides[port.name]
            guard let label, !label.trimmingCharacters(in: .whitespaces).isEmpty else {
                return port
            }
            return AudioPort(id: port.id, name: port.name, shortName: port.shortName,
                              channelRange: port.channelRange, color: port.color, rmeInterface: label)
        }
    }

    private static func colorForFamily(_ family: String) -> ConnectorType {
        switch family.uppercased() {
        case "MADI":  return .madi
        case "ADAT":  return .adat
        case "AES":   return .aes
        case "SPDIF": return .spdif
        case "DANTE": return .dante
        case "PHONES", "LINE", "INSTRUMENT": return .analogLine
        case "MIC":   return .analogMic
        case "USB":   return .usb
        default:      return .other
        }
    }
}

// MARK: - Data Model

struct ParsedDevice: Identifiable, Hashable {
    let id = UUID()
    let name: String
    let type: DeviceType
    let sendChannels: [Int]   // Dispatcher busses — Cubase → hardware (OUTPUT from Cubase)
    let recvChannels: [Int]   // Receiver busses  — hardware → Cubase (INPUT to Cubase)
    let midiDevice: String

    enum DeviceType: String, Hashable {
        case instrument = "Instrument"
        case fx         = "External FX"
        case sendOnly   = "FX Send Only"
    }

    var allChannels: [Int]  { Array(Set(sendChannels + recvChannels)).sorted() }
    var isStereo: Bool      { recvChannels.count == 2 || sendChannels.count == 2 }
    var isMono: Bool        { allChannels.count == 1 || (recvChannels.count == 1 && sendChannels.count <= 1) }
    var channelCount: Int   { allChannels.count }
    var hasMismatchedChannels: Bool {
        !sendChannels.isEmpty && !recvChannels.isEmpty &&
        sendChannels.sorted() != recvChannels.sorted()
    }

    var primaryPort: AudioPort? {
        let ch = recvChannels.first ?? sendChannels.first
        return ch.flatMap { AudioPortLayout.port(for: $0) }
    }

    var ports: Set<AudioPort> {
        Set(allChannels.compactMap { AudioPortLayout.port(for: $0) })
    }
}

// MARK: - Channel direction (for patchbay two-row display)

enum ChannelDirection {
    case send   // Cubase output → hardware
    case recv   // Hardware output → Cubase input
}

struct ParsedDevices {
    let instruments: [ParsedDevice]
    let fxDevices:   [ParsedDevice]
    var all: [ParsedDevice] { instruments + fxDevices }

    /// All channels regardless of direction
    var channelMap: [Int: [ParsedDevice]] {
        var map: [Int: [ParsedDevice]] = [:]
        for dev in all {
            for ch in dev.allChannels {
                map[ch, default: []].append(dev)
            }
        }
        return map
    }

    /// Send channels only (Cubase → hardware, i.e. what comes OUT of Cubase)
    var sendChannelMap: [Int: [ParsedDevice]] {
        var map: [Int: [ParsedDevice]] = [:]
        for dev in all {
            for ch in dev.sendChannels {
                map[ch, default: []].append(dev)
            }
        }
        return map
    }

    /// Recv channels only (hardware → Cubase, i.e. what comes INTO Cubase)
    var recvChannelMap: [Int: [ParsedDevice]] {
        var map: [Int: [ParsedDevice]] = [:]
        for dev in all {
            for ch in dev.recvChannels {
                map[ch, default: []].append(dev)
            }
        }
        return map
    }

    var usedChannelCount: Int { channelMap.keys.count }

    var activePorts: [AudioPort] {
        let usedPorts = Set(all.flatMap { $0.ports })
        return AudioPortLayout.ports.filter { usedPorts.contains($0) }
    }
}



// MARK: - Parser

enum ExternalPluginsParser {

    enum ParseError: Error, LocalizedError {
        case invalidXML(String)
        case missingRootElement

        var errorDescription: String? {
            switch self {
            case .invalidXML(let msg): return "Invalid XML: \(msg)"
            case .missingRootElement:  return "Missing <ExternalPlugins> root element"
            }
        }
    }

    static func parse(data: Data) throws -> ParsedDevices {
        let xml = try XMLDocument(data: data, options: [])
        guard let root = xml.rootElement() else { throw ParseError.missingRootElement }

        guard let effectsList = root.elements(forName: "list").first(where: {
            $0.attribute(forName: "name")?.stringValue == "External Effects"
        }) else { return ParsedDevices(instruments: [], fxDevices: []) }

        var instruments: [ParsedDevice] = []
        var fxDevices:   [ParsedDevice] = []

        for item in effectsList.elements(forName: "item") {
            guard let dev = parseItem(item) else { continue }
            switch dev.type {
            case .instrument:         instruments.append(dev)
            case .fx, .sendOnly:      fxDevices.append(dev)
            }
        }

        return ParsedDevices(instruments: instruments, fxDevices: fxDevices)
    }

    // MARK: - Item parsing

    private static func parseItem(_ item: XMLElement) -> ParsedDevice? {
        // Name
        guard let name = string(in: item, named: "External Effect Name"), !name.isEmpty else { return nil }

        // Type
        let isInstrument = int(in: item, named: "External Instrument") == 1

        // MIDI
        let midiPath = item
            .elements(forName: "member").first { $0.attribute(forName: "name")?.stringValue == "Midi Link" }
            .flatMap { $0.elements(forName: "string").first { $0.attribute(forName: "name")?.stringValue == "NodePathID" } }
            .flatMap { $0.attribute(forName: "value")?.stringValue } ?? ""
        let midiDevice = midiPath
            .replacingOccurrences(of: "Midi Device Manager\\", with: "")

        // Send channels (Dispatcher Busses)
        let sendChannels = channels(from: item, busListName: "Dispatcher Busses")

        // Receive channels (Receiver Busses)
        let recvChannels = channels(from: item, busListName: "Receiver Busses")

        // Skip completely empty devices (iMac Tiger etc.)
        if sendChannels.isEmpty && recvChannels.isEmpty && !isInstrument { return nil }

        let type: ParsedDevice.DeviceType
        if isInstrument {
            type = .instrument
        } else if recvChannels.isEmpty {
            type = .sendOnly
        } else {
            type = .fx
        }

        return ParsedDevice(
            name: name,
            type: type,
            sendChannels: sendChannels,
            recvChannels: recvChannels,
            midiDevice: midiDevice
        )
    }

    // MARK: - Helpers

    private static func channels(from item: XMLElement, busListName: String) -> [Int] {
        var result: [Int] = []
        for busList in item.elements(forName: "list") where
            busList.attribute(forName: "name")?.stringValue == busListName {
            for busItem in busList.elements(forName: "item") {
                for connList in busItem.elements(forName: "list") where
                    connList.attribute(forName: "name")?.stringValue == "Connections" {
                    for connItem in connList.elements(forName: "item") {
                        guard let val = connItem.attribute(forName: "value")?.stringValue,
                              val.contains("|") else { continue }
                        let parts = val.components(separatedBy: "|")
                        guard let last = parts.last else { continue }
                        let chStr = last.replacingOccurrences(of: "HDSPe Device ", with: "").trimmingCharacters(in: .whitespaces)
                        if let ch = Int(chStr) { result.append(ch) }
                    }
                }
            }
        }
        return result.sorted()
    }

    private static func string(in element: XMLElement, named attrName: String) -> String? {
        element.elements(forName: "string")
            .first { $0.attribute(forName: "name")?.stringValue == attrName }
            .flatMap { $0.attribute(forName: "value")?.stringValue }
    }

    private static func int(in element: XMLElement, named attrName: String) -> Int? {
        element.elements(forName: "int")
            .first { $0.attribute(forName: "name")?.stringValue == attrName }
            .flatMap { $0.attribute(forName: "value")?.stringValue }
            .flatMap { Int($0) }
    }
}
