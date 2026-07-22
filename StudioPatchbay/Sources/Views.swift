import SwiftUI
import AppKit

extension ConnectorType {
    var color: Color {
        switch self {
        case .madi:       return Color(red: 0.35, green: 0.65, blue: 1.0)
        case .adat:       return Color(red: 0.4,  green: 0.85, blue: 0.55)
        case .aes:        return Color(red: 1.0,  green: 0.72, blue: 0.3)
        case .spdif:      return Color(red: 1.0,  green: 0.55, blue: 0.85)
        case .dante:      return Color(red: 0.55, green: 0.85, blue: 1.0)
        case .usb:        return Color(red: 0.7,  green: 0.7,  blue: 0.75)
        case .thunderbolt:return Color(red: 0.5,  green: 0.55, blue: 1.0)
        case .analogLine: return Color(red: 0.95, green: 0.60, blue: 0.25)
        case .analogMic:  return Color(red: 0.85, green: 0.35, blue: 0.35)
        case .other:      return .secondary
        }
    }
}
extension AudioPort { var displayColor: Color { color.color } }

extension Color {
    /// Parses a "#RRGGBB" hex string. Returns nil for anything malformed, so callers
    /// can safely fall back to a default color rather than crash on bad stored data.
    init?(hex: String) {
        var s = hex.trimmingCharacters(in: .whitespacesAndNewlines)
        if s.hasPrefix("#") { s.removeFirst() }
        guard s.count == 6, let value = UInt32(s, radix: 16) else { return nil }
        let r = Double((value >> 16) & 0xFF) / 255
        let g = Double((value >> 8) & 0xFF) / 255
        let b = Double(value & 0xFF) / 255
        self = Color(red: r, green: g, blue: b)
    }

    /// Best-effort "#RRGGBB" conversion for persisting a user-picked color.
    func toHex() -> String {
        let nsColor = NSColor(self).usingColorSpace(.deviceRGB) ?? NSColor(self)
        let r = Int(round(nsColor.redComponent * 255))
        let g = Int(round(nsColor.greenComponent * 255))
        let b = Int(round(nsColor.blueComponent * 255))
        return String(format: "#%02X%02X%02X", r, g, b)
    }
}

// MARK: - Sidebar

struct SidebarView: View {
    @ObservedObject var state: AppState
    @Binding var filterMode: FilterMode
    @Binding var searchText: String
    @Binding var viewMode: ContentView.ViewMode
    @Environment(\.openWindow) private var openWindow

    var body: some View {
        List(selection: $filterMode) {
            versionSection
            filterSection
            viewSection
            settingsSection
        }
        .listStyle(.sidebar)
        .navigationTitle("Studio Patchbay")
    }

    private var settingsSection: some View {
        Section {
            Button(action: { openOrFocusSettingsWindow { openWindow(id: "physicalDeviceSettings") } }) {
                Label("Setup Converters & Interfaces", systemImage: "slider.horizontal.3")
                    .foregroundStyle(Color.secondary)
            }
            .buttonStyle(.plain)
        }
    }

    private var versionSection: some View {
        Section("Cubase Version") {
            if state.installations.isEmpty {
                Label("No installations found", systemImage: "exclamationmark.triangle")
                    .font(.caption).foregroundStyle(.secondary)
            } else {
                ForEach(state.installations) { inst in
                    InstallationRowView(
                        inst: inst,
                        isSelected: state.selectedInstallation == inst,
                        onSelect: { state.select(inst) }
                    )
                }
            }
        }
    }

    private var filterSection: some View {
        Section("Filter") {
            ForEach(FilterMode.allCases) { mode in
                Label(mode.rawValue, systemImage: mode.icon).tag(mode)
            }
        }
    }

    private var viewSection: some View {
        Section("View") {
            ForEach(ContentView.ViewMode.allCases, id: \.self) { mode in
                Button(action: { viewMode = mode }) {
                    Label(mode.rawValue, systemImage: mode.icon)
                        .foregroundStyle(viewMode == mode ? Color.primary : Color.secondary)
                }
                .buttonStyle(.plain)
            }
        }
    }
}

extension ContentView.ViewMode {
    var icon: String {
        switch self {
        case .cards:       return "square.grid.2x2"
        case .patchbay:    return "square.grid.4x3.fill"
        case .controlRoom: return "dial.high.fill"
        }
    }
}

struct InstallationRowView: View {
    let inst: CubaseInstallation
    let isSelected: Bool
    let onSelect: () -> Void

    private var dotFill: Color    { isSelected ? .green : .clear }
    private var dotBorder: Color  { isSelected ? .green : Color.secondary.opacity(0.4) }
    private var subText: String   { inst.xmlExists ? "External Plugins.xml found" : "No XML — launch Cubase first" }
    private var subColor: Color   { inst.xmlExists ? .secondary : .red }

    var body: some View {
        Button(action: { guard inst.xmlExists else { return }; onSelect() }) {
            HStack(spacing: 8) {
                Circle().fill(dotFill).frame(width: 7, height: 7)
                    .overlay(Circle().strokeBorder(dotBorder, lineWidth: 1))
                VStack(alignment: .leading, spacing: 1) {
                    Text(inst.displayName).font(.callout)
                        .foregroundStyle(inst.xmlExists ? Color.primary : Color.secondary)
                    Text(subText).font(.caption2).foregroundStyle(subColor)
                }
                Spacer()
            }
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
        .disabled(!inst.xmlExists)
        .padding(.vertical, 2)
    }
}

// MARK: - Cards View

struct CardsView: View {
    let instruments: [ParsedDevice]
    let fxDevices:   [ParsedDevice]
    let cellScale:   Double
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool

    private var cardMin: CGFloat { CGFloat(200 * cellScale) }
    private var cardMax: CGFloat { CGFloat(360 * cellScale) }
    private var columns: [GridItem] { [GridItem(.adaptive(minimum: cardMin, maximum: cardMax), spacing: 10)] }

    private var regularFX:  [ParsedDevice] { fxDevices.filter { $0.type == .fx } }
    private var sendOnlyFX: [ParsedDevice] { fxDevices.filter { $0.type == .sendOnly } }
    private var isEmpty:    Bool           { instruments.isEmpty && fxDevices.isEmpty }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                provenanceCaption
                if !instruments.isEmpty {
                    sectionHeader("Instruments", count: instruments.count, color: .blue)
                    deviceGrid(instruments)
                }
                if !regularFX.isEmpty {
                    sectionHeader("External FX", count: regularFX.count, color: .orange)
                    deviceGrid(regularFX)
                }
                if !sendOnlyFX.isEmpty {
                    sectionHeader("FX Sends (Synth Routing)", count: sendOnlyFX.count, color: .purple)
                    deviceGrid(sendOnlyFX)
                }
                if isEmpty {
                    ContentUnavailableView("No matches", systemImage: "magnifyingglass",
                        description: Text("Try adjusting your search or filter."))
                }
            }
            .padding(16)
        }
    }

    private var provenanceCaption: some View {
        Text("From Cubase's External Instruments & External Effects (Studio > Audio Connections)")
            .font(.caption2).foregroundStyle(.tertiary)
    }

    private func deviceGrid(_ devices: [ParsedDevice]) -> some View {
        LazyVGrid(columns: columns, spacing: 10) {
            ForEach(devices) { dev in
                DeviceCardView(device: dev, isSelected: selectedDevice == dev, scale: cellScale)
                    .onTapGesture { selectDevice(dev) }
            }
        }
    }

    private func selectDevice(_ dev: ParsedDevice) {
        if selectedDevice == dev {
            selectedDevice = nil  // deselect — closes the detail panel
        } else {
            selectedDevice = dev
        }
    }

    private func sectionHeader(_ title: String, count: Int, color: Color) -> some View {
        HStack(spacing: 8) {
            Text(title).font(.callout.weight(.semibold)).foregroundStyle(color)
            Text("\(count)").font(.caption.monospaced())
                .padding(.horizontal, 6).padding(.vertical, 1)
                .background(.quaternary, in: Capsule()).foregroundStyle(.secondary)
            Rectangle().fill(.separator).frame(height: 0.5)
        }
    }
}

// MARK: - Device Card

struct DeviceCardView: View {
    let device: ParsedDevice
    let isSelected: Bool
    var scale: Double = 1.0
    @State private var isHovering = false

    var accentColor: Color {
        switch device.type {
        case .instrument: return .blue
        case .fx:         return .orange
        case .sendOnly:   return .purple
        }
    }

    private var borderWidth: CGFloat { isSelected ? 1.5 : (isHovering ? 1 : 0.5) }
    private var borderColor: Color {
        isSelected ? accentColor : (isHovering ? accentColor.opacity(0.5) : Color.primary.opacity(0.1))
    }

    // Scale-aware font sizes
    private var nameSize:    Font { .system(size: CGFloat(14 * scale), weight: .semibold) }
    private var typeSize:    Font { .system(size: CGFloat(11 * scale)) }
    private var chLabelSize: Font { .system(size: CGFloat(10 * scale), weight: .medium, design: .monospaced) }
    private var chNumSize:   Font { .system(size: CGFloat(11 * scale), design: .monospaced) }
    private var badgeSize:   Font { .system(size: CGFloat(9  * scale), weight: .semibold, design: .monospaced) }
    private var midiSize:    Font { .system(size: CGFloat(11 * scale), design: .monospaced) }

    private var formatLabel: String {
        if device.allChannels.count > 2 { return "\(device.allChannels.count)ch" }
        if device.isStereo { return "Stereo" }
        if device.isMono   { return "Mono" }
        return ""
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            cardHeader
            Divider()
            channelRows
            deviceSummary
            midiRow
            mismatchWarning
        }
        .padding(12)
        .background(cardBackground)
        .overlay(alignment: .leading, content: accentStripe)
        .scaleEffect(isHovering ? 1.015 : 1.0)
        .animation(.easeOut(duration: 0.12), value: isHovering)
        .onHover { isHovering = $0 }
        .pointerStyle(.link)
    }

    // Reflects whatever converters/interfaces are currently configured in Settings.
    // Present when a channel resolves to one, absent otherwise — so this naturally
    // updates as devices are added, edited, or removed, with no separate
    // empty-state needed here.
    @ViewBuilder private var deviceSummary: some View {
        let outNames = Set(device.sendChannels.compactMap { PhysicalDeviceLayout.device(for: $0, direction: .send)?.name })
        let inNames  = Set(device.recvChannels.compactMap { PhysicalDeviceLayout.device(for: $0, direction: .recv)?.name })
        if !outNames.isEmpty || !inNames.isEmpty {
            VStack(alignment: .leading, spacing: 1) {
                if !outNames.isEmpty {
                    Text("OUT: \(outNames.sorted().joined(separator: ", "))")
                        .font(.system(size: CGFloat(9.5 * scale))).foregroundStyle(.secondary)
                        .lineLimit(1).truncationMode(.tail)
                }
                if !inNames.isEmpty {
                    Text("IN: \(inNames.sorted().joined(separator: ", "))")
                        .font(.system(size: CGFloat(9.5 * scale))).foregroundStyle(.secondary)
                        .lineLimit(1).truncationMode(.tail)
                }
            }
        }
    }

    private var cardHeader: some View {
        HStack(alignment: .top, spacing: 6) {
            VStack(alignment: .leading, spacing: 3) {
                Text(device.name).font(nameSize).lineLimit(2)
                HStack(spacing: 4) {
                    Text(device.type.rawValue).font(typeSize).foregroundStyle(accentColor)
                    ForEach(Array(device.ports).sorted(by: { $0.id < $1.id })) { port in
                        portBadge(port)
                    }
                }
            }
            Spacer()
            if !formatLabel.isEmpty {
                Text(formatLabel).font(badgeSize)
                    .padding(.horizontal, 5).padding(.vertical, 2)
                    .background(accentColor.opacity(0.12), in: RoundedRectangle(cornerRadius: 4))
                    .foregroundStyle(accentColor)
            }
        }
    }

    private func portBadge(_ port: AudioPort) -> some View {
        Text(port.shortName).font(badgeSize)
            .padding(.horizontal, 4).padding(.vertical, 1)
            .background(port.displayColor.opacity(0.15), in: RoundedRectangle(cornerRadius: 3))
            .foregroundStyle(port.displayColor)
    }

    @ViewBuilder
    private var channelRows: some View {
        VStack(alignment: .leading, spacing: 4) {
            if device.allChannels.isEmpty {
                noChannelsWarning
            } else if device.type == .instrument {
                if !device.recvChannels.isEmpty {
                    channelRow(label: "↓ IN", channels: device.recvChannels, color: .blue)
                }
            } else {
                if !device.sendChannels.isEmpty {
                    let c: Color = device.type == .sendOnly ? .purple : .orange
                    channelRow(label: "↑ SEND", channels: device.sendChannels, color: c)
                }
                if !device.recvChannels.isEmpty {
                    channelRow(label: "↓ RECV", channels: device.recvChannels, color: .blue)
                }
            }
        }
    }

    private var noChannelsWarning: some View {
        Label("No channels assigned", systemImage: "exclamationmark.triangle.fill")
            .font(typeSize).foregroundStyle(.orange)
    }

    private func channelRow(label: String, channels: [Int], color: Color) -> some View {
        HStack(alignment: .top, spacing: 6) {
            Text(label).font(chLabelSize).foregroundStyle(.tertiary)
                .frame(width: CGFloat(38 * scale), alignment: .leading)
            ChannelBadgeFlow(spacing: 3, lineSpacing: 3) {
                ForEach(channels, id: \.self) { ch in
                    Text("\(ch)").font(chNumSize)
                        .fixedSize()
                        .padding(.horizontal, 5).padding(.vertical, 1)
                        .background(color.opacity(0.12), in: RoundedRectangle(cornerRadius: 3))
                        .foregroundStyle(color)
                }
            }
        }
    }

    @ViewBuilder private var midiRow: some View {
        if !device.midiDevice.isEmpty {
            HStack(spacing: 4) {
                Circle().fill(Color.green).frame(width: 5, height: 5)
                Text(device.midiDevice).font(midiSize).foregroundStyle(.secondary)
            }
        }
    }

    @ViewBuilder private var mismatchWarning: some View {
        if device.hasMismatchedChannels {
            Label("Send/return channels differ", systemImage: "exclamationmark.triangle.fill")
                .font(typeSize).foregroundStyle(.orange)
        }
    }

    private var cardBackground: some View {
        RoundedRectangle(cornerRadius: 9).fill(.background)
            .overlay(RoundedRectangle(cornerRadius: 9).strokeBorder(borderColor, lineWidth: borderWidth))
            .shadow(color: .black.opacity(isHovering ? 0.12 : 0.06), radius: isHovering ? 7 : 4, y: isHovering ? 3 : 2)
    }

    private func accentStripe() -> some View {
        RoundedRectangle(cornerRadius: 2).fill(accentColor)
            .frame(width: 3).padding(.vertical, 8).padding(.leading, 1)
    }
}

// Wraps channel badges onto additional rows when they don't fit — without this,
// a plain HStack starts compressing individual Text views once there isn't room for
// all of them on one line, wrapping the NUMBER itself onto multiple lines (e.g.
// "211" rendering as "2"/"1"/"1" stacked) instead of moving whole badges down.
// Devices with enough channels to overflow a card's width (8+ is common) need this.
private struct ChannelBadgeFlow: Layout {
    var spacing: CGFloat = 3
    var lineSpacing: CGFloat = 3

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        let maxWidth = proposal.width ?? .infinity
        var x: CGFloat = 0, y: CGFloat = 0, rowHeight: CGFloat = 0
        for subview in subviews {
            let size = subview.sizeThatFits(.unspecified)
            if x > 0, x + size.width > maxWidth {
                x = 0
                y += rowHeight + lineSpacing
                rowHeight = 0
            }
            x += size.width + spacing
            rowHeight = max(rowHeight, size.height)
        }
        y += rowHeight
        return CGSize(width: maxWidth.isFinite ? maxWidth : x, height: y)
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        var x: CGFloat = bounds.minX, y: CGFloat = bounds.minY, rowHeight: CGFloat = 0
        for subview in subviews {
            let size = subview.sizeThatFits(.unspecified)
            if x > bounds.minX, x + size.width > bounds.minX + bounds.width {
                x = bounds.minX
                y += rowHeight + lineSpacing
                rowHeight = 0
            }
            subview.place(at: CGPoint(x: x, y: y), proposal: ProposedViewSize(size))
            x += size.width + spacing
            rowHeight = max(rowHeight, size.height)
        }
    }
}

// MARK: - Patchbay View

struct PatchbayView: View {
    let devices: ParsedDevices
    let highlightedDevices: [ParsedDevice]
    let cellScale: Double
    let isSearchActive: Bool          // true only when user has typed a search query
    var controlRoomBuses: [ControlRoomBus] = []
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool
    @Binding var scrollRequest: ScrollRequest?
    // Export-only — see PhysicalDeviceGroupView.showConnectedDevices.
    var showConnectedDevices: Bool = false
    @Environment(\.openWindow) private var openWindow

    var highlightedChannels: Set<Int> {
        Set(highlightedDevices.flatMap { $0.allChannels })
    }

    /// Ports to render — the union of ports touched by External Plugins devices and
    /// ports touched by Control Room buses. Without this, a port used only for
    /// monitoring (like the MADI FX AES pair feeding the Genelec system) would never
    /// appear, since no External Plugins device sits on it.
    ///
    /// Auto-detected ports can legitimately overlap — e.g. "Phones" (send-only,
    /// direct hardware output) sits entirely inside "ADAT"'s channel range, since
    /// they're really two different numbering pools coinciding on the same raw
    /// channel numbers for different directions (the same phenomenon we handle at
    /// the converter level with direction-aware ownership). Left unhandled, that
    /// means BOTH ports get their own PortSectionView, and anything in the
    /// overlapping range renders twice. This drops any port whose entire channel
    /// range is already covered by an earlier (lower-starting) port, so each channel
    /// only ever gets rendered under one port section.
    private var activePorts: [AudioPort] {
        let fromDevices = Set(devices.activePorts)
        let fromControlRoom = Set(
            controlRoomBuses.flatMap(\.channels).compactMap { AudioPortLayout.port(for: $0) }
        )
        let candidates = AudioPortLayout.ports.filter { fromDevices.contains($0) || fromControlRoom.contains($0) }
            .sorted { $0.channelRange.lowerBound < $1.channelRange.lowerBound }

        var claimed = Set<Int>()
        var result: [AudioPort] = []
        for port in candidates {
            let isFullyClaimed = port.channelRange.allSatisfy { claimed.contains($0) }
            if isFullyClaimed { continue }
            claimed.formUnion(port.channelRange)
            result.append(port)
        }
        return result
    }

    // Same content the scrollable body wraps in a ScrollView — exposed separately
    // (not private) so the exporter can render it directly via ImageRenderer at its
    // full natural height, with no scroll clipping and no interactive dependency on
    // being inside an actual window.
    var exportableContent: some View {
        VStack(alignment: .leading, spacing: 0) {
            if PhysicalDeviceLayout.devices.isEmpty {
                noDevicesBanner
                Divider()
            }
            patchbayLegend
            Divider()
            ForEach(activePorts) { port in
                PortSectionView(
                    port: port,
                    sendMap: devices.sendChannelMap,
                    recvMap: devices.recvChannelMap,
                    highlightedChannels: highlightedChannels,
                    cellScale: cellScale,
                    isSearchActive: isSearchActive,
                    controlRoomMap: controlRoomBuses.channelMap,
                    selectedDevice: $selectedDevice,
                    showDetail: $showDetail,
                    showConnectedDevices: showConnectedDevices
                )
            }
            AudioInterfaceSectionView(
                devices: PhysicalDeviceLayout.devices.filter { $0.deviceType == .audioInterface }
                    .sorted { $0.sendChannelStart < $1.sendChannelStart },
                sendMap: devices.sendChannelMap,
                recvMap: devices.recvChannelMap,
                highlightedChannels: highlightedChannels,
                cellScale: cellScale,
                isSearchActive: isSearchActive,
                controlRoomMap: controlRoomBuses.channelMap,
                selectedDevice: $selectedDevice,
                showDetail: $showDetail,
                showConnectedDevices: showConnectedDevices
            )
            ControlRoomInPatchbaySection(
                buses: controlRoomBuses,
                sendMap: devices.sendChannelMap,
                recvMap: devices.recvChannelMap,
                highlightedChannels: highlightedChannels,
                cellScale: cellScale,
                isSearchActive: isSearchActive,
                selectedDevice: $selectedDevice,
                showDetail: $showDetail
            )
        }
        .padding(.bottom, 20)
    }

    var body: some View {
        ScrollViewReader { proxy in
            ScrollView {
                exportableContent
            }
            .onChange(of: scrollRequest) { _, newValue in
                guard let request = newValue else { return }
                withAnimation(.easeInOut(duration: 0.35)) {
                    proxy.scrollTo("channel-\(request.channel)", anchor: .center)
                }
            }
        }
    }

    // No devices configured at all yet — every real channel still
    // shows (as raw "Unknown" blocks, since that's accurate: nothing has been told
    // what physical hardware they belong to), but without this it's not obvious
    // *why* everything looks unlabeled rather than that something's broken.
    private var noDevicesBanner: some View {
        HStack(spacing: 10) {
            Image(systemName: "cable.connector.slash").foregroundStyle(.orange)
            Text("No converters or interfaces configured yet — channels below show without physical device grouping until you add some in Settings.")
                .font(.caption).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
            Spacer()
            Button("Open Settings…") { openOrFocusSettingsWindow { openWindow(id: "physicalDeviceSettings") } }
                .font(.caption)
        }
        .padding(.horizontal, 16).padding(.vertical, 8)
        .background(Color.orange.opacity(0.08))
    }

    private var patchbayLegend: some View {
        HStack(spacing: 16) {
            legendItem(color: .blue,   label: "Instrument")
            legendItem(color: .orange, label: "FX")
            legendItem(color: .purple, label: "Send only")
            legendItem(color: .teal,   label: "Control Room")
            Divider().frame(height: 14)
            Text("Each block = one converter or interface  ·  OUT row on top, IN row below  ·  Hover for details  ·  Click to inspect")
                .font(.caption2).foregroundStyle(.tertiary)
            Spacer()
        }
        .padding(.horizontal, 16).padding(.vertical, 10)
    }

    private func legendItem(color: Color, label: String) -> some View {
        HStack(spacing: 5) {
            RoundedRectangle(cornerRadius: 2).fill(color.opacity(0.25))
                .overlay(RoundedRectangle(cornerRadius: 2).strokeBorder(color.opacity(0.6), lineWidth: 0.5))
                .frame(width: 14, height: 14)
            Text(label).font(.caption2).foregroundStyle(.secondary)
        }
    }
}

// MARK: - Port section (OUT row + converter strip + IN row)

struct PortSectionView: View {
    let port: AudioPort
    let sendMap: [Int: [ParsedDevice]]
    let recvMap:  [Int: [ParsedDevice]]
    let highlightedChannels: Set<Int>
    let cellScale: Double
    let isSearchActive: Bool
    var controlRoomMap: [Int: [ControlRoomBus]] = [:]
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool
    var showConnectedDevices: Bool = false

    private var usedSendMax: Int {
        port.channelRange
            .filter { sendMap[$0] != nil && !PhysicalDeviceLayout.isChannelOwned($0, byDeviceType: .audioInterface) }
            .max() ?? port.channelRange.lowerBound
    }
    private var usedRecvMax: Int {
        port.channelRange
            .filter { recvMap[$0] != nil && !PhysicalDeviceLayout.isChannelOwned($0, byDeviceType: .audioInterface) }
            .max() ?? port.channelRange.lowerBound
    }
    private var usedControlRoomMax: Int {
        port.channelRange.filter { controlRoomMap[$0] != nil }.max() ?? port.channelRange.lowerBound
    }
    private var displayEnd: Int {
        let raw = max(usedSendMax, usedRecvMax, usedControlRoomMax)
        return min(((raw + 15) / 16) * 16, port.channelRange.upperBound)
    }
    private var usedCount: Int {
        let s = Set(port.channelRange.filter { sendMap[$0] != nil })
        let r = Set(port.channelRange.filter { recvMap[$0] != nil })
        return s.union(r).count
    }

    private var deviceGroups: [(device: PhysicalDevice?, range: ClosedRange<Int>)] {
        var groups: [(PhysicalDevice?, ClosedRange<Int>)] = []
        // Tracks devices already given a group via the recv scan below, so the
        // send-fallback (meant only for devices with NO recv presence at all,
        // e.g. an output-only monitor system) never creates a SECOND group for a
        // device whose send range simply extends a bit further than its recv
        // range (a bidirectional device with an asymmetric offset, like the
        // RayDAT case) — without this, that trailing send-only tail got matched
        // again by the fallback and rendered as a duplicate block.
        var seenDeviceIDs = Set<UUID>()
        var i = port.channelRange.lowerBound
        while i <= displayEnd {
            // Audio Interface devices are standalone USB/Thunderbolt devices with
            // no relationship to the RME's own port map — they get their own
            // dedicated Patchbay section instead, so their channels are skipped here
            // entirely (not matched, and not even shown as an unlabeled "Unknown"
            // block, which would otherwise misleadingly suggest they're part of this
            // RME port's topology).
            if PhysicalDeviceLayout.isChannelOwned(i, byDeviceType: .audioInterface) {
                i += 1
                continue
            }
            // Grouping is based on RECV ownership specifically: recv ranges are
            // contiguous and non-overlapping end-to-end (they mirror the physical
            // ADAT/MADI port's own channel count exactly), unlike the union
            // channelRange, which can overlap between asymmetric neighbors (e.g. the
            // Evo SP8's send range reaches into channels the next device uses for
            // recv). Each block's OUT row still reads its own device's actual send
            // range independently — see PhysicalDeviceGroupView.
            if let dev = PhysicalDeviceLayout.device(for: i, direction: .recv, excluding: .audioInterface) {
                let end = min(dev.range(for: .recv).upperBound, displayEnd)
                if !seenDeviceIDs.contains(dev.id) {
                    groups.append((dev, i...end))
                    seenDeviceIDs.insert(dev.id)
                }
                i = end + 1
            } else if let dev = PhysicalDeviceLayout.device(for: i, direction: .send, excluding: .audioInterface) {
                // Output-only devices (e.g. a monitor system with no input path at
                // all) have no recv presence for the scan above to find — fall back to
                // send ownership so they still get their own block. Whatever's on the
                // recv side of this same physical connector isn't this device's
                // concern; it'll correctly render with no device attribution.
                let end = min(dev.range(for: .send).upperBound, displayEnd)
                if !seenDeviceIDs.contains(dev.id) {
                    groups.append((dev, i...end))
                    seenDeviceIDs.insert(dev.id)
                }
                i = end + 1
            } else {
                // Unmapped channel (no hardcoded device covers it). Only worth
                // showing if something is actually using it — an External Plugins
                // device or a Control Room bus. A channel nobody uses and no
                // physical device claims is just unused hardware capacity; hide it
                // rather than clutter the view with an empty "Unknown" block per channel.
                if sendMap[i] != nil || recvMap[i] != nil || controlRoomMap[i] != nil {
                    groups.append((nil, i...i))
                }
                i += 1
            }
        }
        return groups
    }

    private var cellSize: CGFloat { max(CGFloat(36 * cellScale), 26) }

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            portHeader
            ForEach(deviceGroups, id: \.range.lowerBound) { group in
                PhysicalDeviceGroupView(
                    device:              group.device,
                    channelRange:        group.range,
                    sendMap:             sendMap,
                    recvMap:             recvMap,
                    highlightedChannels: highlightedChannels,
                    portColor:           port.displayColor,
                    rmeInterface:        port.rmeInterface,
                    cellSize:            cellSize,
                    cellScale:           cellScale,
                    isSearchActive:      isSearchActive,
                    controlRoomMap:      controlRoomMap,
                    selectedDevice:      $selectedDevice,
                    showDetail:          $showDetail,
                    showConnectedDevices: showConnectedDevices
                )
                .padding(.horizontal, 12)
                Divider().opacity(0.35)
            }
            Divider()
        }
    }

    private var portHeader: some View {
        HStack(spacing: 8) {
            RoundedRectangle(cornerRadius: 3)
                .fill(port.displayColor.opacity(0.3))
                .overlay(RoundedRectangle(cornerRadius: 3)
                    .strokeBorder(port.displayColor.opacity(0.7), lineWidth: 1))
                .frame(width: 8, height: 8)
            Text(port.name)
                .font(.caption.weight(.semibold))
                .foregroundStyle(port.displayColor)
            Text("·  \(port.rmeInterface)")
                .font(.caption2).foregroundStyle(.secondary)
            Text("ch \(port.channelRange.lowerBound)–\(port.channelRange.upperBound)")
                .font(.caption2.monospaced()).foregroundStyle(.tertiary)
            Spacer()
            Text("\(usedCount) channels in use").font(.caption2).foregroundStyle(.tertiary)
        }
        .padding(.horizontal, 16).padding(.vertical, 8)
        .background(port.displayColor.opacity(0.07))
        .overlay(alignment: .bottom) { Divider() }
    }
}

// MARK: - Audio Interface section (standalone devices, separate from RME topology)

/// Renders Audio Interface devices (self-contained USB/Thunderbolt devices like
/// an RME, Universal Audio, or MOTU box) in their own section, independent of the
/// RME-topology port sections above. These have no relationship to the RME's own
/// MADI/ADAT/AES port map — grouping them under an RME port header would be
/// meaningless (and PortSectionView excludes their channels for exactly that
/// reason), so each one gets its own standalone block here instead.
struct AudioInterfaceSectionView: View {
    let devices: [PhysicalDevice]
    let sendMap: [Int: [ParsedDevice]]
    let recvMap: [Int: [ParsedDevice]]
    let highlightedChannels: Set<Int>
    let cellScale: Double
    let isSearchActive: Bool
    var controlRoomMap: [Int: [ControlRoomBus]] = [:]
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool
    var showConnectedDevices: Bool = false

    private var cellSize: CGFloat { max(CGFloat(36 * cellScale), 26) }

    var body: some View {
        if !devices.isEmpty {
            VStack(alignment: .leading, spacing: 0) {
                sectionHeader
                Divider()
                ForEach(devices) { dev in
                    PhysicalDeviceGroupView(
                        device: dev,
                        channelRange: dev.channelRange,
                        sendMap: sendMap,
                        recvMap: recvMap,
                        highlightedChannels: highlightedChannels,
                        portColor: dev.portColor.color,
                        rmeInterface: "Audio Interface",
                        cellSize: cellSize,
                        cellScale: cellScale,
                        isSearchActive: isSearchActive,
                        controlRoomMap: controlRoomMap,
                        selectedDevice: $selectedDevice,
                        showDetail: $showDetail,
                        showConnectedDevices: showConnectedDevices
                    )
                }
            }
        }
    }

    private var sectionHeader: some View {
        HStack(spacing: 8) {
            Image(systemName: "cpu").font(.caption).foregroundStyle(.secondary)
            Text("Audio Interfaces").font(.caption.weight(.semibold))
            Text("·  Standalone devices, separate from the RME's own port map")
                .font(.caption2).foregroundStyle(.secondary)
            Spacer()
        }
        .padding(.horizontal, 16).padding(.vertical, 8)
        .background(Color.primary.opacity(0.05))
        .overlay(alignment: .bottom) { Divider() }
    }
}

// MARK: - Control Room section

/// Renders every Control Room bus as its own block — its own section, not just the
/// teal badges overlaid on other devices' OUT cells. Those badges stay exactly as
/// they are (a bus using a channel that also belongs to some converter still shows
/// both), but a bus needs a home of its own regardless of whether its channels
/// happen to be claimed by a neighboring converter's channel numbering. That's
/// exactly the gap a bus like a direct Phones output used to fall through: it sits
/// on real hardware channels no converter's own send range ever claims.
struct ControlRoomInPatchbaySection: View {
    let buses: [ControlRoomBus]
    let sendMap: [Int: [ParsedDevice]]
    let recvMap: [Int: [ParsedDevice]]
    let highlightedChannels: Set<Int>
    let cellScale: Double
    let isSearchActive: Bool
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool

    private var cellSize: CGFloat { max(CGFloat(36 * cellScale), 26) }

    var body: some View {
        if !buses.isEmpty {
            VStack(alignment: .leading, spacing: 0) {
                sectionHeader
                Divider()
                ForEach(buses) { bus in
                    ControlRoomBusRowView(
                        bus: bus,
                        sendMap: sendMap,
                        recvMap: recvMap,
                        highlightedChannels: highlightedChannels,
                        cellSize: cellSize,
                        cellScale: cellScale,
                        isSearchActive: isSearchActive,
                        selectedDevice: $selectedDevice,
                        showDetail: $showDetail
                    )
                    Divider().opacity(0.35)
                }
            }
        }
    }

    private var sectionHeader: some View {
        HStack(spacing: 8) {
            Image(systemName: "dial.high.fill").font(.caption).foregroundStyle(.secondary)
            Text("Control Room").font(.caption.weight(.semibold))
            Text("·  Monitor bus outputs, downstream of the interfaces and converters above")
                .font(.caption2).foregroundStyle(.secondary)
            Spacer()
        }
        .padding(.horizontal, 16).padding(.vertical, 8)
        .background(Color.teal.opacity(0.06))
        .overlay(alignment: .bottom) { Divider() }
    }
}

/// One Control Room bus, rendered like a device block but output-only (no IN row —
/// a monitor bus doesn't receive anything back into Cubase) and noting what it's
/// actually routed through, if anything, cross-referencing PhysicalDeviceLayout the
/// same way Signal Flow does from the other direction.
struct ControlRoomBusRowView: View {
    let bus: ControlRoomBus
    let sendMap: [Int: [ParsedDevice]]
    let recvMap: [Int: [ParsedDevice]]
    let highlightedChannels: Set<Int>
    let cellSize: CGFloat
    let cellScale: Double
    let isSearchActive: Bool
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool

    private var cellGap: CGFloat { 3 }
    private var labelSize:  CGFloat { max(CGFloat(9  * cellScale), 8.5) }
    private var headerSize: CGFloat { max(CGFloat(12 * cellScale), 10.5) }
    private var subSize:    CGFloat { max(CGFloat(9  * cellScale), 8.0) }

    private var sortedChannels: [Int] { bus.channels.sorted() }

    // Which converter/interface (if any) this bus's channels actually route
    // through — a bus can legitimately span more than one if its channels don't
    // all belong to the same physical device.
    private var routingLabel: String {
        let names = Set(sortedChannels.compactMap { PhysicalDeviceLayout.device(for: $0, direction: .send)?.name })
        if names.isEmpty { return "direct hardware output (no converter/interface)" }
        return "via " + names.sorted().joined(separator: " + ")
    }

    private var rangeText: String {
        guard let first = sortedChannels.first, let last = sortedChannels.last else { return "" }
        let isContiguous = sortedChannels == Array(first...last)
        return (isContiguous && sortedChannels.count > 1) ? "\(first)–\(last)" : sortedChannels.map(String.init).joined(separator: "+")
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            header
            dirLabel("OUT → Hardware", color: .teal.opacity(0.7)).padding(.top, 5)
            cellRow
        }
        .padding(.vertical, 6)
        .padding(.horizontal, 12)
    }

    private var header: some View {
        HStack(spacing: 0) {
            RoundedRectangle(cornerRadius: 2).fill(Color.teal).frame(width: 3).padding(.trailing, 8)
            VStack(alignment: .leading, spacing: 1) {
                HStack(spacing: 5) {
                    Image(systemName: bus.kind.icon).font(.system(size: headerSize * 0.75))
                    Text(bus.name).font(.system(size: headerSize, weight: .bold))
                }
                .foregroundStyle(.teal)
                Text("\(bus.kind.label)  ·  OUT ch \(rangeText)  ·  \(routingLabel)")
                    .font(.system(size: subSize, design: .monospaced))
                    .foregroundStyle(.teal.opacity(0.65))
            }
            Spacer()
        }
        .padding(.vertical, 6).padding(.horizontal, 8)
        .background(Color.teal.opacity(0.09))
        .overlay(alignment: .bottom) { Rectangle().fill(Color.teal.opacity(0.25)).frame(height: 0.5) }
        .overlay(alignment: .top) { Rectangle().fill(Color.teal.opacity(0.35)).frame(height: 1) }
    }

    private func dirLabel(_ text: String, color: Color) -> some View {
        Text(text)
            .font(.system(size: labelSize, weight: .semibold, design: .monospaced))
            .foregroundStyle(color)
            .padding(.bottom, 2)
    }

    private var cellRow: some View {
        HStack(spacing: cellGap) {
            ForEach(sortedChannels, id: \.self) { ch in
                ChannelCellView(
                    channel: ch,
                    devices: sendMap[ch],
                    direction: .send,
                    isHighlighted: highlightedChannels.isEmpty || highlightedChannels.contains(ch),
                    isSearchActive: isSearchActive,
                    cellWidth: cellSize,
                    physicalIO: PhysicalDeviceLayout.device(for: ch, direction: .send)?.physicalIO(for: ch, direction: .send),
                    controlRoomBuses: [bus],
                    selectedDevice: $selectedDevice,
                    showDetail: $showDetail
                )
            }
            Spacer(minLength: 0)
        }
    }
}

// One device row — full window width, OUT on top, header in middle, IN below
struct PhysicalDeviceGroupView: View {
    let device: PhysicalDevice?
    let channelRange: ClosedRange<Int>
    let sendMap: [Int: [ParsedDevice]]
    let recvMap: [Int: [ParsedDevice]]
    let highlightedChannels: Set<Int>
    let portColor: Color
    let rmeInterface: String
    let cellSize: CGFloat
    let cellScale: Double
    let isSearchActive: Bool
    var controlRoomMap: [Int: [ControlRoomBus]] = [:]
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool
    // Export-only: appends a compact "Connected: ..." line listing the Cubase
    // devices actually using this block's channels, so a printed export answers
    // "what's connected here" right where the block is, instead of needing a
    // separate cross-reference table. Defaults to false — the interactive Patchbay
    // is completely unaffected unless the exporter explicitly opts in.
    var showConnectedDevices: Bool = false

    private var channels: [Int]    { Array(channelRange) }
    private var cellGap:  CGFloat  { 3 }
    private var accentColor: Color {
        guard let dev = device else { return .secondary }
        if let hex = dev.customColorHex, let custom = Color(hex: hex) { return custom }
        return portColor
    }

    private var labelSize:  CGFloat { max(CGFloat(9  * cellScale), 8.5) }
    private var headerSize: CGFloat { max(CGFloat(12 * cellScale), 10.5) }

    /// The channels to render for one row. Reads the device's own send or recv span
    /// directly (not the shared union `channelRange`), so OUT and IN always show exactly
    /// `physicalChannelCount` cells each, numbered 1…N — naturally aligned regardless of
    /// whether the device's send and recv channel numbers differ (asymmetric devices
    /// like the Evo SP8). Falls back to the raw block channel(s) — same as an "Unknown"
    /// block — when there's no device at all, OR when this block's device doesn't
    /// actually support this direction (e.g. an output-only monitor system's IN row):
    /// that channel isn't going through any conversion, so it correctly shows with no
    /// device attribution rather than being mislabeled.
    private func channelsForRow(direction: ChannelDirection) -> [Int] {
        guard let dev = device, dev.supports(direction) else { return channels }
        return Array(dev.range(for: direction))
    }
    private var subSize:    CGFloat { max(CGFloat(9  * cellScale), 8.0) }

    // Every Cubase device actually using one of this block's channels, in either
    // direction — deduped (a device can straddle both OUT and IN of the same
    // block) and sorted by its first channel for a natural reading order.
    private var connectedDevices: [ParsedDevice] {
        let sendChannels = channelsForRow(direction: .send)
        let recvChannels = channelsForRow(direction: .recv)
        var seen = Set<UUID>()
        var result: [ParsedDevice] = []
        for ch in sendChannels + recvChannels {
            for dev in (sendMap[ch] ?? []) + (recvMap[ch] ?? []) where !seen.contains(dev.id) {
                seen.insert(dev.id)
                result.append(dev)
            }
        }
        return result.sorted { ($0.allChannels.first ?? 0) < ($1.allChannels.first ?? 0) }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // ── Device header at TOP — anchors the whole row ──────
            deviceHeader
            // ── OUT row ──────────────────────────────────────────────
            dirLabel("OUT → Hardware", color: accentColor.opacity(0.7))
                .padding(.top, 5)
            cellRow(map: sendMap, direction: .send)
            // ── IN row ───────────────────────────────────────────────
            dirLabel("IN ← Hardware", color: accentColor)
                .padding(.top, 4)
            cellRow(map: recvMap, direction: .recv)
            if showConnectedDevices {
                connectedDevicesFooter
            }
        }
        .padding(.vertical, 6)
        .fixedSize(horizontal: false, vertical: true)
    }

    private var connectedDevicesFooter: some View {
        Group {
            if !connectedDevices.isEmpty || !connectedControlRoomBuses.isEmpty {
                VStack(alignment: .leading, spacing: 3) {
                    Text("Connected")
                        .font(.system(size: labelSize, weight: .semibold, design: .monospaced))
                        .foregroundStyle(accentColor.opacity(0.6))
                        .padding(.bottom, 2)

                    HStack(spacing: 12) {
                        Text("Device").frame(width: 210, alignment: .leading)
                        Text("Type").frame(width: 100, alignment: .leading)
                        Text("OUT").frame(width: 80, alignment: .leading)
                        Text("IN")
                    }
                    .font(.system(size: subSize, weight: .semibold))
                    .foregroundStyle(.secondary.opacity(0.8))
                    Divider().opacity(0.3)

                    ForEach(connectedDevices) { dev in
                        let (out, inn) = blockChannelStrings(for: dev)
                        HStack(spacing: 12) {
                            Text(dev.name)
                                .frame(width: 210, alignment: .leading)
                                .lineLimit(1)
                            Text(dev.type.rawValue)
                                .frame(width: 100, alignment: .leading)
                                .lineLimit(1)
                            Text(out)
                                .frame(width: 80, alignment: .leading)
                            Text(inn)
                        }
                        .font(.system(size: subSize, design: .monospaced))
                        .foregroundStyle(.secondary)
                    }

                    // Control Room buses are a completely separate data source
                    // (controlRoomMap, not sendMap/recvMap) — a monitor bus
                    // assignment, not a Cubase External Plugins device. Output-only
                    // by nature (they feed hardware monitors, nothing comes back),
                    // so the IN column is always blank for these rows.
                    ForEach(connectedControlRoomBuses) { bus in
                        HStack(spacing: 12) {
                            Text(bus.name)
                                .frame(width: 210, alignment: .leading)
                                .lineLimit(1)
                            Text("Control Room")
                                .frame(width: 100, alignment: .leading)
                                .lineLimit(1)
                            Text(blockChannelString(for: bus))
                                .frame(width: 80, alignment: .leading)
                            Text("")
                        }
                        .font(.system(size: subSize, design: .monospaced))
                        .foregroundStyle(.teal)
                    }
                }
                .padding(.top, 6)
            }
        }
    }

    // Every Control Room bus whose OUTPUT touches one of this block's channels —
    // Control Room only ever assigns to the send/OUT direction (it feeds hardware
    // monitors, nothing comes back into Cubase from it), so only that direction
    // is checked. Deduped and sorted the same way as connectedDevices.
    private var connectedControlRoomBuses: [ControlRoomBus] {
        var seen = Set<Int>()
        var result: [ControlRoomBus] = []
        for ch in channelsForRow(direction: .send) {
            for bus in controlRoomMap[ch] ?? [] where !seen.contains(bus.id) {
                seen.insert(bus.id)
                result.append(bus)
            }
        }
        return result.sorted { ($0.channels.first ?? 0) < ($1.channels.first ?? 0) }
    }

    private func blockChannelString(for bus: ControlRoomBus) -> String {
        let blockSend = Set(channelsForRow(direction: .send))
        let relevant = bus.channels.filter { blockSend.contains($0) }.sorted()
        return rangeString(relevant)
    }

    // Separate OUT and IN channel strings, each scoped to just the channels of
    // this device that fall within THIS block (not the device's full channel
    // list — a device split across two neighboring blocks, like RD-8 spanning
    // both the Evo SP8 and Ferrofish A16 MKII/3 blocks, would otherwise show its
    // whole range in both places, which doesn't explain why it's listed under
    // either specific block). Kept separate rather than combined, since an
    // Instrument's channels are typically recv-only while an FX device usually
    // has both — a merged list can't say which direction a given channel is.
    private func blockChannelStrings(for dev: ParsedDevice) -> (out: String, inn: String) {
        let blockSend = Set(channelsForRow(direction: .send))
        let blockRecv = Set(channelsForRow(direction: .recv))
        let devSend = dev.sendChannels.filter { blockSend.contains($0) }.sorted()
        let devRecv = dev.recvChannels.filter { blockRecv.contains($0) }.sorted()
        return (rangeString(devSend), rangeString(devRecv))
    }

    private func rangeString(_ channels: [Int]) -> String {
        guard let first = channels.first, let last = channels.last else { return "" }
        let isContiguous = channels == Array(first...last)
        return (isContiguous && channels.count > 1) ? "\(first)–\(last)" : channels.map(String.init).joined(separator: "+")
    }

    private func dirLabel(_ text: String, color: Color) -> some View {
        Text(text)
            .font(.system(size: labelSize, weight: .semibold, design: .monospaced))
            .foregroundStyle(color)
            .padding(.bottom, 2)
    }

    // Kept as a plain function (not inline in a ViewBuilder closure) — a switch used
    // purely for computing a String, embedded directly inside a View's body closure,
    // gets misinterpreted by the ViewBuilder result builder as needing to produce a
    // View per case, which fails to compile ("Type '()' cannot conform to 'View'").
    private func headerChannelLabel(for dev: PhysicalDevice) -> String {
        let sendR = dev.range(for: .send)
        let recvR = dev.range(for: .recv)
        switch dev.directionality {
        case .sendOnly:
            return "OUT ch \(sendR.lowerBound)–\(sendR.upperBound)"
        case .recvOnly:
            return "IN ch \(recvR.lowerBound)–\(recvR.upperBound)"
        case .bidirectional:
            return sendR == recvR
                ? "ch \(recvR.lowerBound)–\(recvR.upperBound)"
                : "OUT ch \(sendR.lowerBound)–\(sendR.upperBound)  ·  IN ch \(recvR.lowerBound)–\(recvR.upperBound)"
        }
    }

    private var deviceHeader: some View {
        HStack(alignment: .top, spacing: 0) {
            // Colored left bar — thicker, full row height visual anchor
            RoundedRectangle(cornerRadius: 2)
                .fill(accentColor)
                .frame(width: 3)
                .padding(.trailing, 8)

            VStack(alignment: .leading, spacing: 1) {
                // Device name — primary, largest text
                HStack(alignment: .firstTextBaseline, spacing: 5) {
                    if let dev = device, dev.deviceType != .converter {
                        Image(systemName: dev.deviceType.icon)
                            .font(.system(size: headerSize * 0.75))
                    }
                    Text(device?.name ?? "Unknown")
                        .font(.system(size: headerSize, weight: .bold))
                }
                .foregroundStyle(accentColor)
                // Interface · channel range · physical I/O — secondary info
                if let dev = device {
                    Text("\(rmeInterface)  ·  \(headerChannelLabel(for: dev))  ·  Physical I/O 1–\(dev.physicalChannelCount)")
                        .font(.system(size: subSize, design: .monospaced))
                        .foregroundStyle(accentColor.opacity(0.65))
                }
            }
            Spacer()
        }
        .padding(.vertical, 6)
        .padding(.horizontal, 8)
        .background(accentColor.opacity(0.09))
        .overlay(alignment: .bottom) {
            Rectangle().fill(accentColor.opacity(0.25)).frame(height: 0.5)
        }
        .overlay(alignment: .top) {
            Rectangle().fill(accentColor.opacity(0.35)).frame(height: 1)
        }
        // Only shows up under ImageRenderer (export), never in the live interactive
        // app — meaning the view itself isn't broken, but ImageRenderer's layout
        // pass is proposing an oversized/ambiguous height for this specific header
        // (plausibly tied to "AES 1" being the only section with just one block,
        // unlike every other section). fixedSize forces this to compute its height
        // from its own content regardless of what height it's offered.
        .fixedSize(horizontal: false, vertical: true)
    }

    // Physical I/O label for the device header subtitle
    // Uses recvChannelStart as the canonical reference for the range display
    private func physicalRangeStr(direction: ChannelDirection) -> String {
        guard let dev = device else { return "" }
        let ios = channels.compactMap { dev.physicalIO(for: $0, direction: direction) }.sorted()
        guard !ios.isEmpty else { return "" }
        if ios.count == 1 { return "I/O \(ios[0])" }
        return "I/O \(ios.first!)–\(ios.last!)"
    }

    private func cellRow(map: [Int: [ParsedDevice]], direction: ChannelDirection) -> some View {
        let rowChannels = channelsForRow(direction: direction)
        return VStack(alignment: .leading, spacing: 1) {
            // Main cell row — physical I/O numbers (the device's perspective)
            HStack(spacing: cellGap) {
                ForEach(rowChannels, id: \.self) { ch in
                    ChannelCellView(
                        channel: ch,
                        devices: map[ch],
                        direction: direction,
                        isHighlighted: highlightedChannels.isEmpty || highlightedChannels.contains(ch),
                        isSearchActive: isSearchActive,
                        cellWidth: cellSize,
                        physicalIO: device?.physicalIO(for: ch, direction: direction),
                        controlRoomBuses: direction == .send ? controlRoomMap[ch] : nil,
                        selectedDevice: $selectedDevice,
                        showDetail: $showDetail
                    )
                    // Only the OUT row needs an id: it's rendered above the IN row within
                    // the same device group, so scrolling to it brings both into view.
                    // Tagging both rows would create duplicate ids for the same channel.
                    .id(direction == .send ? "channel-\(ch)" : nil)
                }
                Spacer(minLength: 0)
            }
            // Sub-label row — MADI/ADAT channel numbers (Cubase's address)
            HStack(spacing: cellGap) {
                ForEach(rowChannels, id: \.self) { ch in
                    Text("ch\(ch)")
                        .font(.system(size: max(CGFloat(9 * cellScale), 8.5), design: .monospaced))
                        .foregroundStyle(accentColor.opacity(0.55))
                        .frame(width: cellSize)
                        .lineLimit(1)
                        .minimumScaleFactor(0.7)
                }
                Spacer(minLength: 0)
            }
        }
        .padding(.bottom, 3)
    }

}


// MARK: - Channel Cell (with fast custom hover tooltip)

struct ChannelCellView: View {
    let channel: Int
    let devices: [ParsedDevice]?
    let direction: ChannelDirection
    let isHighlighted: Bool
    let isSearchActive: Bool   // true when user has typed a search query
    let cellWidth: CGFloat
    var physicalIO: Int? = nil    // when set, shown as primary; channel shown in tooltip only
    var controlRoomBuses: [ControlRoomBus]? = nil   // Control Room monitor buses using this channel (OUT row only)
    @Binding var selectedDevice: ParsedDevice?
    @Binding var showDetail: Bool

    @State private var isHovered = false
    @State private var hoverFrame: CGRect = .zero

    private var isUsed: Bool { devices != nil }
    private var crBuses: [ControlRoomBus] { controlRoomBuses ?? [] }
    private var isControlRoomOnly: Bool { !crBuses.isEmpty && !isUsed }
    private var isSelected: Bool {
        guard let sel = selectedDevice, let devs = devices else { return false }
        return devs.contains(sel)
    }
    private var isActive: Bool { isHovered || isSelected }
    private var directionDim: Double { direction == .send ? 0.82 : 1.0 }

    // The number shown large inside the cell
    private var displayNumber: String {
        if let pio = physicalIO { return "\(pio)" }
        return "\(channel)"
    }

    private var bgColor: Color {
        guard let devs = devices, !devs.isEmpty else {
            if isControlRoomOnly { return Color(red: 0.06, green: 0.28, blue: 0.26).opacity(directionDim) }
            return Color(white: direction == .send ? 0.18 : 0.22)
        }
        let types = Set(devs.map(\.type))
        if types.contains(.instrument) && !types.contains(.fx) {
            return Color(red: 0.12, green: 0.22, blue: 0.42).opacity(directionDim)
        }
        if types.contains(.sendOnly) && types.count == 1 {
            return Color(red: 0.22, green: 0.10, blue: 0.35).opacity(directionDim)
        }
        return Color(red: 0.38, green: 0.22, blue: 0.04).opacity(directionDim)
    }

    private var borderColor: Color {
        guard let devs = devices, !devs.isEmpty else {
            if isControlRoomOnly { return .teal.opacity(0.75) }
            return Color(white: direction == .send ? 0.28 : 0.32)
        }
        let types = Set(devs.map(\.type))
        if types.contains(.instrument) && !types.contains(.fx) { return .blue.opacity(0.8) }
        if types.contains(.sendOnly) && types.count == 1 { return .purple.opacity(0.8) }
        return .orange.opacity(0.8)
    }

    private var fgColor: Color {
        guard let devs = devices, !devs.isEmpty else {
            if isControlRoomOnly { return Color(red: 0.55, green: 0.95, blue: 0.90).opacity(directionDim) }
            return Color(white: direction == .send ? 0.42 : 0.48)
        }
        let types = Set(devs.map(\.type))
        if types.contains(.instrument) && !types.contains(.fx) {
            return Color(red: 0.55, green: 0.80, blue: 1.0).opacity(directionDim)
        }
        if types.contains(.sendOnly) && types.count == 1 {
            return Color(red: 0.80, green: 0.60, blue: 1.0).opacity(directionDim)
        }
        return Color(red: 1.0, green: 0.72, blue: 0.30).opacity(directionDim)
    }

    private var cellOpacity: Double    { guard isUsed else { return 1 }; return isHighlighted ? 1 : 0.15 }
    private var activeBg: Color        { isActive && isUsed ? Color.white.opacity(0.12) : .clear }
    private var activeBorderColor: Color {
        // Search match: bright white ring takes priority
        if isSearchActive && isHighlighted && isUsed { return Color.white.opacity(0.9) }
        guard isUsed && isActive else { return borderColor }
        return isSelected ? Color.white.opacity(0.9) : Color.white.opacity(0.6)
    }
    private var activeBorderWidth: CGFloat {
        if isSearchActive && isHighlighted && isUsed { return 1.5 }
        guard isUsed && isActive else { return 0.5 }
        return isSelected ? 1.5 : 1.0
    }
    private var activeScale: CGFloat   { isActive && isUsed ? 1.05 : 1.0 }
    private var fontSize: CGFloat      { min(max(cellWidth * 0.28, 8), 16) }

    private var tooltipText: String? {
        let dir = direction == .send ? "OUT" : "IN"
        let chInfo = physicalIO != nil ? "I/O \(physicalIO!)  ·  MADI/ADAT ch \(channel)" : "ch \(channel)"

        var lines: [String] = []
        if let devs = devices, !devs.isEmpty {
            lines.append("[\(dir)]  \(chInfo)")
            if devs.count == 1 {
                lines.append(devs[0].name)
            } else {
                lines.append(contentsOf: devs.map { "• \($0.name)" })
                lines.append("(tap to cycle)")
            }
        }
        if !crBuses.isEmpty {
            if lines.isEmpty { lines.append("[\(dir)]  \(chInfo)") }
            lines.append(contentsOf: crBuses.map { "🎛 Control Room: \($0.name)" })
        }
        return lines.isEmpty ? nil : lines.joined(separator: "\n")
    }

    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 4).fill(bgColor)
                .overlay(RoundedRectangle(cornerRadius: 4).fill(activeBg))
                .overlay(RoundedRectangle(cornerRadius: 4).strokeBorder(activeBorderColor, lineWidth: activeBorderWidth))
            Text(displayNumber)
                .font(.system(size: fontSize, design: .monospaced))
                .foregroundStyle(fgColor)
                .lineLimit(1)
                .minimumScaleFactor(0.7)
            if let bus = crBuses.first {
                VStack {
                    HStack {
                        Spacer()
                        Image(systemName: bus.kind.icon)
                            .font(.system(size: max(cellWidth * 0.22, 6)))
                            .foregroundStyle(.teal)
                            .padding(2)
                    }
                    Spacer()
                }
            }
        }
        .frame(width: cellWidth, height: cellWidth)
        .opacity(cellOpacity)
        .scaleEffect(activeScale)
        .animation(.easeOut(duration: 0.08), value: isHovered)
        .animation(.easeOut(duration: 0.08), value: isSelected)
        .onHover { hovering in isHovered = hovering }
        .onTapGesture(perform: handleTap)
        .disabled(!isUsed && crBuses.isEmpty)
        // Tooltip uses .overlay on a zero-size anchor to escape the cell's clip bounds
        .overlay(alignment: .topLeading) {
            if isHovered, let tip = tooltipText {
                Color.clear.frame(width: 0, height: 0)
                    .overlay(alignment: .bottomLeading) {
                        TooltipView(text: tip)
                            .fixedSize()
                            .offset(y: -cellWidth - 8)
                    }
                    .zIndex(999)
                    .allowsHitTesting(false)
            }
        }
    }

    private func handleTap() {
        guard let devs = devices, !devs.isEmpty else { return }
        if devs.count == 1 {
            // Toggle: tap selected device deselects it
            selectedDevice = (selectedDevice == devs.first) ? nil : devs.first
        } else {
            // Multiple devices on channel: cycle through them
            if let current = selectedDevice, let idx = devs.firstIndex(of: current) {
                let next = devs[(idx + 1) % devs.count]
                selectedDevice = (next == current) ? nil : next
            } else {
                selectedDevice = devs.first
            }
        }
    }
}

// Immediate tooltip bubble — plain View, no OS involvement
struct TooltipView: View {
    let text: String

    var body: some View {
        Text(text)
            .font(.system(size: 11))
            .multilineTextAlignment(.leading)
            .padding(.horizontal, 8)
            .padding(.vertical, 5)
            .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .strokeBorder(Color.primary.opacity(0.12), lineWidth: 0.5)
            )
            .shadow(color: .black.opacity(0.25), radius: 6, y: 3)
    }
}

// MARK: - Device Detail

struct DeviceDetailView: View {
    let device: ParsedDevice
    let onClose: () -> Void
    var onJumpToChannel: (Int) -> Void = { _ in }

    var accentColor: Color {
        switch device.type {
        case .instrument: return .blue
        case .fx:         return .orange
        case .sendOnly:   return .purple
        }
    }

    private var formatString: String {
        if device.allChannels.count > 2 { return "\(device.allChannels.count)-channel" }
        if device.isStereo { return "Stereo" }
        if device.isMono   { return "Mono" }
        return "—"
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                detailHeader
                Divider()
                VStack(alignment: .leading, spacing: 8) {
                    sectionTitle("Signal Flow")
                    SignalFlowView(device: device, accentColor: accentColor, onJumpToChannel: onJumpToChannel)
                }
                Divider()
                VStack(alignment: .leading, spacing: 8) {
                    sectionTitle("Details")
                    infoGrid
                }
                Spacer(minLength: 0)
            }
            .padding(16)
        }
        .background(.background.secondary)
        .frame(maxHeight: .infinity)
    }

    private var detailHeader: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: 2) {
                    Text(device.type.rawValue.uppercased())
                        .font(.caption2.weight(.semibold)).foregroundStyle(accentColor).tracking(1)
                    Text(device.name).font(.title3.weight(.semibold))
                }
                Spacer()
                Button(action: onClose) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundStyle(.secondary)
                        .font(.title3)
                }
                .buttonStyle(.plain)
            }
            HStack(spacing: 4) {
                ForEach(Array(device.ports).sorted(by: { $0.id < $1.id })) { port in
                    HStack(spacing: 4) {
                        Circle().fill(port.displayColor).frame(width: 6, height: 6)
                        Text(port.name).font(.caption2).foregroundStyle(port.displayColor)
                    }
                    .padding(.horizontal, 6).padding(.vertical, 2)
                    .background(port.displayColor.opacity(0.12), in: Capsule())
                }
            }
        }
        .padding(.bottom, 4)
    }

    private func sectionTitle(_ text: String) -> some View {
        Text(text).font(.caption.weight(.semibold)).foregroundStyle(.secondary)
            .textCase(.uppercase).tracking(0.5)
    }

    private var infoGrid: some View {
        let sortedPorts = Array(device.ports).sorted(by: { $0.id < $1.id })
        let channelLabel: String = {
            let colors = Set(sortedPorts.map(\.color))
            if colors == [.adat] { return "ADAT Channels" }
            if colors == [.madi] { return "MADI Channels" }
            return "Channels"
        }()

        // Gather device + physical I/O info per direction
        let sendDeviceRows = deviceInfoRows(for: device.sendChannels, direction: .send)
        let recvDeviceRows = deviceInfoRows(for: device.recvChannels, direction: .recv)

        return VStack(spacing: 4) {
            infoRow("Format", value: formatString)
            if !device.midiDevice.isEmpty { infoRow("MIDI Device", value: device.midiDevice) }
            if !device.allChannels.isEmpty {
                infoRow(channelLabel, value: device.allChannels.map(String.init).joined(separator: ", "))
            }
            ForEach(sortedPorts) { port in infoRow("Port", value: port.name) }

            // Physical device + physical I/O
            if !sendDeviceRows.isEmpty {
                ForEach(sendDeviceRows, id: \.label) { row in
                    infoRow(row.label, value: row.value)
                }
            }
            if !recvDeviceRows.isEmpty && recvDeviceRows != sendDeviceRows {
                ForEach(recvDeviceRows, id: \.label) { row in
                    infoRow(row.label, value: row.value)
                }
            }

            infoRow("Type", value: device.type.rawValue)
        }
    }

    private struct InfoRow: Equatable {
        let label: String
        let value: String
    }

    private func deviceInfoRows(for channels: [Int], direction: ChannelDirection) -> [InfoRow] {
        guard !channels.isEmpty else { return [] }
        var seen = Set<UUID>()
        var rows: [InfoRow] = []
        for ch in channels {
            guard let dev = PhysicalDeviceLayout.device(for: ch, direction: direction), !seen.contains(dev.id) else { continue }
            seen.insert(dev.id)
            let dirLabel = direction == .send ? "OUT" : "IN"
            let ioLabel = dev.physicalRangeLabel(for: channels.filter { dev.contains($0, direction: direction) }, direction: direction) ?? "I/O ?"
            rows.append(InfoRow(label: "\(dev.deviceType.displayName) (\(dirLabel))", value: dev.name))
            rows.append(InfoRow(label: "Physical I/O (\(dirLabel))", value: ioLabel))
        }
        return rows
    }

    private func infoRow(_ label: String, value: String) -> some View {
        HStack {
            Text(label).foregroundStyle(.secondary)
            Spacer()
            Text(value).font(.callout.monospaced()).foregroundStyle(.primary)
        }
        .font(.callout)
        .padding(.horizontal, 10).padding(.vertical, 7)
        .background(.quaternary.opacity(0.5), in: RoundedRectangle(cornerRadius: 5))
    }
}

// MARK: - Signal Flow

struct SignalFlowView: View {
    let device: ParsedDevice
    let accentColor: Color
    var onJumpToChannel: (Int) -> Void = { _ in }

    // Group channels by physical device, preserving order, with physical I/O label
    private struct PhysicalDeviceIO {
        let device: PhysicalDevice
        let channels: [Int]
        let direction: ChannelDirection
        var physicalLabel: String { device.physicalRangeLabel(for: channels, direction: direction) ?? "" }
        var fullLabel: String { "\(device.name)  —  \(physicalLabel)" }
    }

    private func deviceIOs(for channels: [Int], direction: ChannelDirection) -> [PhysicalDeviceIO] {
        var seen = Set<UUID>()
        var result: [PhysicalDeviceIO] = []
        for ch in channels {
            guard let dev = PhysicalDeviceLayout.device(for: ch, direction: direction) else { continue }
            if seen.contains(dev.id) {
                if let idx = result.firstIndex(where: { $0.device.id == dev.id }) {
                    result[idx] = PhysicalDeviceIO(device: dev, channels: result[idx].channels + [ch], direction: direction)
                }
            } else {
                seen.insert(dev.id)
                result.append(PhysicalDeviceIO(device: dev, channels: [ch], direction: direction))
            }
        }
        return result
    }

    private var recvIOs: [PhysicalDeviceIO] { deviceIOs(for: device.recvChannels, direction: .recv) }
    private var sendIOs: [PhysicalDeviceIO] { deviceIOs(for: device.sendChannels, direction: .send) }

    private var recvChStr: String { device.recvChannels.map(String.init).joined(separator: " + ") }
    private var sendChStr: String { device.sendChannels.map(String.init).joined(separator: " + ") }

    // Derive the RME interface name from the device's actual port assignment
    private var recvInterface: String {
        let ch = device.recvChannels.first ?? device.sendChannels.first ?? 0
        return AudioPortLayout.port(for: ch)?.rmeInterface ?? "RME HDSPe"
    }
    private var sendInterface: String {
        let ch = device.sendChannels.first ?? device.recvChannels.first ?? 0
        return AudioPortLayout.port(for: ch)?.rmeInterface ?? "RME HDSPe"
    }

    var body: some View {
        VStack(spacing: 2) {
            switch device.type {
            case .instrument: instrumentFlow
            case .sendOnly:   sendOnlyFlow
            case .fx:         fxFlow
            }
        }
    }

    @ViewBuilder private var instrumentFlow: some View {
        flowNode(icon: "🖥", label: "Cubase instrument track", color: .primary)
        if !device.midiDevice.isEmpty {
            flowArrow
            flowNode(icon: "🎹", label: "MIDI out → \(device.midiDevice)", color: .green)
        }
        flowArrow
        flowNode(icon: "🎹", label: device.name, color: accentColor)
        if device.recvChannels.isEmpty {
            flowArrow
            flowNode(icon: "⚠️", label: "No channels assigned in Cubase", color: .orange)
        } else {
            flowArrow
            ForEach(recvIOs, id: \.device.id) { cio in
                flowNode(icon: "🔌", label: "Analog out → \(cio.fullLabel)", color: .secondary, jumpChannel: cio.channels.first)
                flowArrow
            }
            let note = recvIOs.isEmpty ? "  ·  no converter/interface configured" : ""
            flowNode(icon: "↓", label: "\(recvInterface)  •  Ch \(recvChStr)\(note)", color: .blue, jumpChannel: device.recvChannels.first)
            flowArrow
            flowNode(icon: "🖥", label: "Cubase — received", color: .primary)
        }
    }

    private var sendOnlyFlow: some View {
        let note = sendIOs.isEmpty ? "  ·  no converter/interface configured" : ""
        return VStack(spacing: 2) {
            flowNode(icon: "🖥", label: "Cubase — sent", color: .primary)
            flowArrow
            flowNode(icon: "↑", label: "\(sendInterface)  •  Ch \(sendChStr)\(note)", color: .purple, jumpChannel: device.sendChannels.first)
            ForEach(sendIOs, id: \.device.id) { cio in
                flowArrow
                flowNode(icon: "🔌", label: "\(cio.fullLabel)  →  Analog in", color: .secondary, jumpChannel: cio.channels.first)
            }
            flowArrow
            flowNode(icon: "🎛", label: device.name, color: accentColor)
        }
    }

    @ViewBuilder private var fxFlow: some View {
        flowNode(icon: "🖥", label: "Cubase FX send", color: .primary)
        if device.sendChannels.isEmpty {
            flowArrow
            flowNode(icon: "⚠️", label: "No send channels assigned", color: .orange)
        } else {
            flowArrow
            let sendNote = sendIOs.isEmpty ? "  ·  no converter/interface configured" : ""
            flowNode(icon: "↑", label: "\(sendInterface)  •  Ch \(sendChStr)\(sendNote)", color: .orange, jumpChannel: device.sendChannels.first)
            ForEach(sendIOs, id: \.device.id) { cio in
                flowArrow
                flowNode(icon: "🔌", label: "\(cio.fullLabel)  →  Analog in", color: .secondary, jumpChannel: cio.channels.first)
            }
        }
        flowArrow
        flowNode(icon: "🎛", label: device.name, color: accentColor)
        flowArrow
        ForEach(recvIOs, id: \.device.id) { cio in
            flowNode(icon: "🔌", label: "Analog out → \(cio.fullLabel)", color: .secondary, jumpChannel: cio.channels.first)
            flowArrow
        }
        let recvNote = recvIOs.isEmpty ? "  ·  no converter/interface configured" : ""
        flowNode(icon: "↓", label: "\(recvInterface)  •  Ch \(recvChStr)\(recvNote)", color: .blue, jumpChannel: device.recvChannels.first)
        flowArrow
        flowNode(icon: "🖥", label: "Cubase FX return", color: .primary)
        if device.hasMismatchedChannels { mismatchBanner }
    }

    private var mismatchBanner: some View {
        HStack(spacing: 6) {
            Image(systemName: "exclamationmark.triangle.fill").foregroundStyle(.orange)
            Text("Send and return channels differ")
        }
        .font(.caption).padding(8)
        .background(.orange.opacity(0.1), in: RoundedRectangle(cornerRadius: 6))
        .padding(.top, 4)
    }

    private func flowNode(icon: String, label: String, color: Color, jumpChannel: Int? = nil) -> some View {
        Group {
            if let ch = jumpChannel {
                ClickableFlowNode(icon: icon, label: label, color: color, cornerRadius: 6) {
                    onJumpToChannel(ch)
                }
            } else {
                HStack(spacing: 10) {
                    Text(icon).frame(width: 22)
                    Text(label).font(.callout).foregroundStyle(.primary)
                    Spacer()
                }
                .padding(9)
                .background(color.opacity(0.08), in: RoundedRectangle(cornerRadius: 6))
                .overlay(RoundedRectangle(cornerRadius: 6).strokeBorder(color.opacity(0.2), lineWidth: 0.5))
            }
        }
    }

    private var flowArrow: some View {
        Text("↓").font(.caption).foregroundStyle(.tertiary).padding(.leading, 12)
    }
}

/// A flow node that jumps to and highlights a channel in the Patchbay when clicked.
/// Shared by SignalFlowView and ControlRoomBusCardView's flow, so both get the same
/// hover affordance (a subtle brightening plus a small arrow icon that only appears
/// on hover — discoverable without needing every node to visually shout "clickable").
private struct ClickableFlowNode: View {
    let icon: String
    let label: String
    let color: Color
    var font: Font = .callout
    var iconWidth: CGFloat = 22
    var innerPadding: CGFloat = 9
    var cornerRadius: CGFloat = 6
    let action: () -> Void
    @State private var isHovering = false

    var body: some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Text(icon).frame(width: iconWidth)
                Text(label).font(font).foregroundStyle(.primary)
                Spacer()
                Image(systemName: "arrow.right.circle")
                    .font(.caption2)
                    .foregroundStyle(color)
                    .opacity(isHovering ? 1 : 0)
            }
            .padding(innerPadding)
            .background(color.opacity(isHovering ? 0.16 : 0.08), in: RoundedRectangle(cornerRadius: cornerRadius))
            .overlay(RoundedRectangle(cornerRadius: cornerRadius).strokeBorder(color.opacity(isHovering ? 0.4 : 0.2), lineWidth: 0.5))
        }
        .buttonStyle(.plain)
        .onHover { isHovering = $0 }
        .help("Jump to this channel in the Patchbay")
    }
}

// MARK: - Control Room View

// A dedicated section for Control Room monitor buses, deliberately independent of
// the Patchbay's device-grid layout. The grid groups cells by device block,
// which works for real AD/DA converters but has no good place to put a channel like
// the MADI FX's direct Phones jacks (ch 195–196) — they're a physical hardware output
// with no device at all, sitting inside another device's channel range for
// unrelated reasons (Cubase's independent send/recv numbering). Listing buses
// directly here, resolved from the Control Room XML with no dependency on which
// Patchbay block happens to claim their channel numbers, means nothing goes missing.
struct ControlRoomView: View {
    let buses: [ControlRoomBus]
    let cellScale: Double

    private var cardMin: CGFloat { CGFloat(280 * cellScale) }
    private var cardMax: CGFloat { CGFloat(420 * cellScale) }
    private var columns: [GridItem] { [GridItem(.adaptive(minimum: cardMin, maximum: cardMax), spacing: 10)] }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                header
                if buses.isEmpty {
                    ContentUnavailableView(
                        "No Control Room setup found",
                        systemImage: "dial.high",
                        description: Text("Set up monitor buses in Cubase's Control Room (Studio > Audio Connections > Studio), then reopen or refresh this app.")
                    )
                    .padding(.top, 40)
                } else {
                    LazyVGrid(columns: columns, spacing: 10) {
                        ForEach(buses) { bus in
                            ControlRoomBusCardView(bus: bus)
                        }
                    }
                }
            }
            .padding(16)
        }
    }

    private var header: some View {
        HStack(spacing: 8) {
            Text("Control Room").font(.callout.weight(.semibold)).foregroundStyle(.teal)
            Text("\(buses.count)").font(.caption.monospaced())
                .padding(.horizontal, 6).padding(.vertical, 1)
                .background(.quaternary, in: Capsule()).foregroundStyle(.secondary)
            Rectangle().fill(.separator).frame(height: 0.5)
        }
    }
}

struct ControlRoomBusCardView: View {
    let bus: ControlRoomBus
    private var accentColor: Color { .teal }

    // Resolves each channel to its real physical device + physical I/O when one
    // exists; falls back to a plain "direct hardware output" description when it
    // doesn't (e.g. the MADI FX's Phones jacks, which are on the card itself, no
    // separate converter box involved).
    private var routes: [String] {
        bus.channels.map { ch in
            if let dev = PhysicalDeviceLayout.device(for: ch, direction: .send),
               let pio = dev.physicalIO(for: ch, direction: .send) {
                return "ch \(ch)  →  \(dev.name)  ·  I/O \(pio)"
            } else if let port = AudioPortLayout.port(for: ch) {
                return "ch \(ch)  →  \(port.rmeInterface)  ·  direct hardware output (no converter)"
            } else {
                return "ch \(ch)"
            }
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            header
            Divider()
            flow
        }
        .padding(12)
        .background(RoundedRectangle(cornerRadius: 8).fill(Color.primary.opacity(0.04)))
        .overlay(RoundedRectangle(cornerRadius: 8).strokeBorder(Color.primary.opacity(0.1), lineWidth: 0.5))
        .overlay(alignment: .leading) {
            RoundedRectangle(cornerRadius: 2).fill(accentColor).frame(width: 3).padding(.vertical, 4)
        }
        .padding(.leading, 4)
    }

    private var header: some View {
        HStack(alignment: .top, spacing: 8) {
            Image(systemName: bus.kind.icon).foregroundStyle(accentColor).frame(width: 18)
            VStack(alignment: .leading, spacing: 2) {
                Text(bus.name).font(.system(size: 14, weight: .semibold)).lineLimit(2)
                Text(bus.kind.label).font(.system(size: 11)).foregroundStyle(accentColor)
            }
            Spacer()
        }
    }

    private var flow: some View {
        VStack(alignment: .leading, spacing: 2) {
            flowNode(icon: "🎛", label: "Cubase Control Room", color: .primary)
            ForEach(Array(routes.enumerated()), id: \.offset) { _, route in
                flowArrow
                flowNode(icon: "↑", label: route, color: accentColor)
            }
        }
    }

    private func flowNode(icon: String, label: String, color: Color) -> some View {
        HStack(spacing: 8) {
            Text(icon).frame(width: 18)
            Text(label).font(.caption).foregroundStyle(.primary)
            Spacer()
        }
        .padding(7)
        .background(color.opacity(0.08), in: RoundedRectangle(cornerRadius: 5))
        .overlay(RoundedRectangle(cornerRadius: 5).strokeBorder(color.opacity(0.2), lineWidth: 0.5))
    }

    private var flowArrow: some View {
        Text("↓").font(.caption2).foregroundStyle(.tertiary).padding(.leading, 9)
    }
}

struct ErrorView: View {
    let message: String
    var body: some View {
        ContentUnavailableView {
            Label("Could not load XML", systemImage: "exclamationmark.triangle")
        } description: { Text(message) }
    }
}

struct EmptyStateView: View {
    let installations: [CubaseInstallation]

    private var descriptionText: String {
        if installations.isEmpty {
            return "No Cubase installations detected in ~/Library/Preferences.\nMake sure Cubase has been launched at least once."
        }
        let names = installations.map(\.displayName).joined(separator: ", ")
        return "Found \(names) but External Plugins.xml is missing.\nOpen Cubase, configure at least one external device, then quit — Cubase writes the file on exit."
    }

    var body: some View {
        ContentUnavailableView {
            Label("No Cubase data found", systemImage: "music.note.house")
        } description: { Text(descriptionText) }
    }
}
