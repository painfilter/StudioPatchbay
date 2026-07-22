import SwiftUI

/// A single "jump to this match" request for the patchbay ScrollView.
/// `token` is a monotonically increasing counter so that tapping the result
/// count twice in a row — even when it resolves to the *same* channel because
/// there's only one match — still triggers a fresh onChange and re-scrolls
/// (an unchanged Equatable value wouldn't fire onChange on its own).
struct ScrollRequest: Equatable {
    let channel: Int
    let token: Int
}

/// Hides the automatic Liquid Glass shared background on macOS 26+; a no-op on
/// earlier macOS (there is no shared glass to hide), so the toolbar degrades
/// gracefully below Tahoe.
private extension ToolbarContent {
    @ToolbarContentBuilder
    func hidingSharedBackgroundIfAvailable() -> some ToolbarContent {
        if #available(macOS 26.0, *) {
            self.sharedBackgroundVisibility(.hidden)
        } else {
            self
        }
    }
}

/// A single point in jump history — just enough to restore "where you were":
/// which top-level view, and which channel (if any) was last scrolled to. The
/// device detail panel itself doesn't change during a jump (only view mode and
/// scroll position do), so that's genuinely all "where you were" means here.
struct NavigationPoint: Equatable {
    let viewMode: ContentView.ViewMode
    let scrollChannel: Int?
}

struct ContentView: View {
    @ObservedObject var state: AppState
    @Environment(\.openWindow) private var openWindow
    @State private var viewMode: ViewMode = .cards
    @State private var filterMode: FilterMode = .all
    @State private var searchText = ""
    @State private var selectedDevice: ParsedDevice?
    @State private var showDetail = false
    @AppStorage("cellScale") private var cellScale: Double = 1.0

    // Search result navigation (Patchbay "jump to next match")
    @State private var searchMatchIndex = -1   // index of the match last jumped to; -1 = none yet
    @State private var scrollRequest: ScrollRequest?
    @State private var scrollToken = 0

    // Back/Forward history for Signal Flow's "jump to channel" — a browser-style
    // pair of stacks. currentScrollChannel tracks the destination of the last jump,
    // so a NEW jump can snapshot "where we were" before moving on.
    @State private var backStack: [NavigationPoint] = []
    @State private var forwardStack: [NavigationPoint] = []
    @State private var currentScrollChannel: Int?

    enum ViewMode: String, CaseIterable {
        case cards = "Cards"
        case patchbay = "Patchbay"
        case controlRoom = "Control Room"
    }

    var body: some View {
        NavigationSplitView {
            SidebarView(
                state: state,
                filterMode: $filterMode,
                searchText: $searchText,
                viewMode: $viewMode
            )
            .navigationSplitViewColumnWidth(min: 200, ideal: 220, max: 260)
        } detail: {
            if let devices = state.parsedDevices {
                // The detail panel used to share HSplitView space with mainContent,
                // shrinking its available width whenever a device was selected. For
                // Cards specifically, that made its adaptive grid recompute layout —
                // even with the column count frozen (a separate fix), each card still
                // narrowed slightly, shifting its exact pixel position enough that a
                // stationary mouse cursor could end up over a different card than the
                // one just clicked. A ZStack overlay alone didn't fully stop this —
                // `maxWidth: .infinity` still left room for its resolved width to
                // differ depending on whether the overlay child was present. Reading
                // the width explicitly via GeometryReader and pinning mainContent to
                // that exact, stable number removes that ambiguity outright.
                GeometryReader { proxy in
                    ZStack(alignment: .trailing) {
                        mainContent(devices: devices)
                            .id(deviceSettingsVersion)   // force full refresh when Settings changes anything
                            .frame(width: proxy.size.width, height: proxy.size.height)

                        if let dev = selectedDevice {
                            DeviceDetailView(device: dev, onClose: {
                                selectedDevice = nil
                                showDetail = false
                            }, onJumpToChannel: { ch in jumpToChannel(ch) })
                            .frame(width: 280)
                            .frame(maxHeight: .infinity)
                            .background(.regularMaterial)
                            .overlay(alignment: .leading) {
                                Divider()
                            }
                            .shadow(color: .black.opacity(0.15), radius: 10, x: -2)
                            .transition(.move(edge: .trailing))
                            .id("\(dev.id)|\(deviceSettingsVersion)")   // force full re-render when device OR devices change
                        }
                    }
                }
            } else if let err = state.errorMessage {
                ErrorView(message: err)
            } else {
                EmptyStateView(installations: state.installations)
            }
        }
        .toolbar {
            toolbarContent
        }
        .onAppear {
            state.refresh()
            state.startWatching()
        }
        .onChange(of: searchText) { _, _ in
            searchMatchIndex = -1
        }
    }

    // MARK: - Main content router

    /// A stable hash of the current physical device list. Cards/Patchbay/Signal Flow
    /// all read device data through a shared static (PhysicalDeviceLayout.devices)
    /// rather than as an explicit tracked input, so nothing in SwiftUI's normal
    /// dependency graph tells it to re-render those views when Settings adds, edits,
    /// deletes, or imports a device — previously this only became visible after
    /// some unrelated action (like clicking Refresh) happened to force the whole
    /// tree to redraw anyway. Using this as an .id() forces a full, correct refresh
    /// the moment devices actually change, without needing that indirect trigger.
    private var deviceSettingsVersion: Int {
        var hasher = Hasher()
        hasher.combine(state.physicalDevices)
        hasher.combine(state.portLabelOverrides)
        return hasher.finalize()
    }

    @ViewBuilder
    private func mainContent(devices: ParsedDevices) -> some View {
        let filtered = filteredDevices(from: devices)
        Group {
            if viewMode == .cards {
                CardsView(
                    instruments: filtered.filter { $0.type == .instrument },
                    fxDevices: filtered.filter { $0.type == .fx || $0.type == .sendOnly },
                    cellScale: cellScale,
                    selectedDevice: $selectedDevice,
                    showDetail: $showDetail
                )
            } else if viewMode == .patchbay {
                PatchbayView(
                    devices: devices,
                    highlightedDevices: filtered,
                    cellScale: cellScale,
                    isSearchActive: !searchText.isEmpty,
                    controlRoomBuses: state.controlRoomBuses,
                    selectedDevice: $selectedDevice,
                    showDetail: $showDetail,
                    scrollRequest: $scrollRequest
                )
            } else {
                ControlRoomView(buses: state.controlRoomBuses, cellScale: cellScale)
            }
        }
    }

    // MARK: - Filtering

    private func filteredDevices(from devices: ParsedDevices) -> [ParsedDevice] {
        var devs = devices.all

        // Search — matches name, MIDI device, channel numbers, and physical device names
        if !searchText.isEmpty {
            let q = searchText.lowercased()
            devs = devs.filter { dev in
                if dev.name.lowercased().contains(q) { return true }
                if dev.midiDevice.lowercased().contains(q) { return true }
                // Match by channel number (exact or partial)
                if dev.allChannels.contains(where: { "\($0)".contains(q) }) { return true }
                // Match by physical device name
                let physicalDeviceNames = dev.allChannels
                    .compactMap { PhysicalDeviceLayout.device(for: $0)?.name.lowercased() }
                if physicalDeviceNames.contains(where: { $0.contains(q) }) { return true }
                return false
            }
        }

        // Filter mode
        switch filterMode {
        case .all:      break
        case .inst:     devs = devs.filter { $0.type == .instrument }
        case .fx:       devs = devs.filter { $0.type == .fx }
        case .sendOnly: devs = devs.filter { $0.type == .sendOnly }
        case .stereo:   devs = devs.filter { $0.isStereo }
        case .mono:     devs = devs.filter { $0.isMono }
        case .midi:     devs = devs.filter { !$0.midiDevice.isEmpty }
        }

        return devs
    }

    /// Search matches sorted in the same top-to-bottom order they appear in the
    /// Patchbay grid (by lowest channel number, so clicking "jump" repeatedly walks
    /// through the grid in a sensible order rather than jumping around randomly).
    private var searchMatches: [ParsedDevice] {
        guard let devices = state.parsedDevices else { return [] }
        return filteredDevices(from: devices).sorted { a, b in
            let ca = a.allChannels.first ?? Int.max
            let cb = b.allChannels.first ?? Int.max
            if ca != cb { return ca < cb }
            return a.name < b.name
        }
    }

    /// Advances to the next search match and scrolls the Patchbay to it, wrapping
    /// around after the last match. Switches to Patchbay mode automatically if the
    /// person is currently in Cards mode, since jumping only makes sense there.
    private func jumpToNextSearchResult() {
        let matches = searchMatches
        guard !matches.isEmpty else { return }

        let nextIndex = (searchMatchIndex + 1) % matches.count
        searchMatchIndex = nextIndex
        guard let channel = matches[nextIndex].allChannels.first else { return }
        jumpToChannel(channel)
    }

    /// Switches to Patchbay (if not already there) and scrolls to a specific
    /// channel — shared by search-result jumping and by clicking a channel node in
    /// Signal Flow. Records where we were first, so Back can return to it.
    private func jumpToChannel(_ channel: Int) {
        backStack.append(NavigationPoint(viewMode: viewMode, scrollChannel: currentScrollChannel))
        forwardStack.removeAll()   // a fresh jump invalidates any old "forward" path
        currentScrollChannel = channel
        performScroll(to: channel)
    }

    private func performScroll(to channel: Int) {
        scrollToken += 1
        let request = ScrollRequest(channel: channel, token: scrollToken)

        if viewMode == .patchbay {
            scrollRequest = request
        } else {
            // Give the Patchbay view a moment to appear before scrolling to it.
            viewMode = .patchbay
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
                scrollRequest = request
            }
        }
    }

    private func navigateBack() {
        guard let point = backStack.popLast() else { return }
        forwardStack.append(NavigationPoint(viewMode: viewMode, scrollChannel: currentScrollChannel))
        restore(point)
    }

    private func navigateForward() {
        guard let point = forwardStack.popLast() else { return }
        backStack.append(NavigationPoint(viewMode: viewMode, scrollChannel: currentScrollChannel))
        restore(point)
    }

    private func restore(_ point: NavigationPoint) {
        currentScrollChannel = point.scrollChannel
        guard let ch = point.scrollChannel else {
            // Nothing was scrolled to yet at this point in history — just the view mode.
            viewMode = point.viewMode
            return
        }
        if viewMode == point.viewMode {
            performScroll(to: ch)
        } else {
            viewMode = point.viewMode
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
                scrollToken += 1
                scrollRequest = ScrollRequest(channel: ch, token: scrollToken)
            }
        }
    }

    // MARK: - Toolbar

    @ToolbarContentBuilder
    private var toolbarContent: some ToolbarContent {
        ToolbarItemGroup(placement: .navigation) {
            Button(action: navigateBack) {
                Image(systemName: "chevron.left")
            }
            .disabled(backStack.isEmpty)
            .help("Back")

            Button(action: navigateForward) {
                Image(systemName: "chevron.right")
            }
            .disabled(forwardStack.isEmpty)
            .help("Forward")
        }

        // Stats + Search in the centre
        // Using ToolbarItemGroup + sharedBackgroundVisibility(.hidden) instead of a plain
        // ToolbarItem: on macOS 26's Liquid Glass toolbar, a single custom ToolbarItem still
        // gets its own automatic glass material rendered behind it. That auto material was
        // layering underneath our manual Capsule() background, producing a double-glass,
        // embossed/3D look. Hiding the shared/automatic background means our Capsule is the
        // ONLY background drawn, so it renders flat — matching the native segmented Picker.
        ToolbarItemGroup(placement: .principal) {
            HStack(spacing: 12) {
                if let devices = state.parsedDevices {
                    statBadge(value: devices.instruments.count, label: "Instruments", color: .blue)
                    statBadge(value: devices.fxDevices.count,  label: "FX Devices",  color: .orange)
                    statBadge(value: devices.usedChannelCount, label: "MADI Channels", color: .secondary)
                }

                Divider().frame(height: 28)

                // Search field
                HStack(spacing: 4) {
                    Image(systemName: "magnifyingglass")
                        .foregroundStyle(.secondary).font(.caption)
                    TextField("Search…", text: $searchText)
                        .textFieldStyle(.plain)
                        .frame(width: 140)
                        .font(.callout)
                        .onKeyPress(.escape) { searchText = ""; return .handled }
                    if !searchText.isEmpty {
                        if let devices = state.parsedDevices {
                            let count = filteredDevices(from: devices).count
                            Button(action: jumpToNextSearchResult) {
                                HStack(spacing: 3) {
                                    Text("\(count)")
                                    if count > 0 {
                                        Image(systemName: "arrow.right.circle.fill")
                                            .font(.caption2)
                                    }
                                }
                                .font(.caption.monospaced())
                                .foregroundStyle(count == 0 ? .red : .secondary)
                                .padding(.horizontal, 5).padding(.vertical, 1)
                                .background(.quaternary, in: Capsule())
                            }
                            .buttonStyle(.plain)
                            .disabled(count == 0)
                            .help(count > 0 ? "Jump to next match in Patchbay" : "")
                        }
                        Button(action: { searchText = "" }) {
                            Image(systemName: "xmark.circle.fill").foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            // Padded enough to fully enclose the two-line stat badges, so the capsule's
            // rounded ends never clip taller content (the original bug you flagged).
            .padding(.horizontal, 14)
            .padding(.vertical, 6)
            .background(Color.primary.opacity(0.06), in: Capsule())
            .fixedSize()
        }
        .hidingSharedBackgroundIfAvailable()

        // Watch status
        // This is the leading-most item in the auto-merged "Updated / Refresh / Zoom"
        // Liquid Glass group, so without a leading inset the circle sits exactly on the
        // pill's rounded left edge and visually pokes outside its curve.
        ToolbarItem(placement: .automatic) {
            HStack(spacing: 5) {
                Circle()
                    .fill(state.isWatching ? Color.green : Color.gray)
                    .frame(width: 7, height: 7)
                if let t = state.lastReloadTime {
                    Text("Updated \(t, style: .relative) ago")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                } else {
                    Text(state.isWatching ? "Watching" : "Not watching")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }
            .padding(.leading, 4)
        }

        // Refresh
        ToolbarItem(placement: .automatic) {
            Button(action: { state.refresh(); state.startWatching() }) {
                Image(systemName: "arrow.clockwise")
            }
            .help("Re-scan Cubase preferences and reload XML")
        }

        // Zoom
        ToolbarItem(placement: .automatic) {
            HStack(spacing: 4) {
                Button(action: { cellScale = max(0.6, cellScale - 0.1) }) {
                    Image(systemName: "minus.magnifyingglass")
                }
                .disabled(cellScale <= 0.6)
                Slider(value: $cellScale, in: 0.6...2.0, step: 0.1)
                    .frame(width: 80)
                Button(action: { cellScale = min(2.0, cellScale + 0.1) }) {
                    Image(systemName: "plus.magnifyingglass")
                }
                .disabled(cellScale >= 2.0)
                Text("\(Int(cellScale * 100))%")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
                    .frame(width: 36, alignment: .trailing)
                Button(action: { cellScale = 1.0 }) {
                    Text("Reset")
                        .font(.caption)
                }
                .foregroundStyle(cellScale == 1.0 ? Color.secondary : Color.accentColor)
                .disabled(cellScale == 1.0)
            }
            .help("Zoom — \(Int(cellScale * 100))%")
        }

        // Fixed gap so the picker capsule isn't crammed against the zoom controls —
        // without this, the picker's left edge sat almost flush against "Reset".
        // (macOS 26 Liquid Glass toolbar only; earlier macOS uses default spacing.)
        if #available(macOS 26.0, *) {
            ToolbarSpacer(.fixed, placement: .automatic)
        }

        // View picker
        ToolbarItem(placement: .automatic) {
            Picker("View", selection: $viewMode) {
                Label("Cards", systemImage: "square.grid.2x2").tag(ViewMode.cards)
                Label("Patchbay", systemImage: "square.grid.4x3.fill").tag(ViewMode.patchbay)
                Label("Control Room", systemImage: "dial.high.fill").tag(ViewMode.controlRoom)
            }
            .pickerStyle(.segmented)
        }

        ToolbarItem(placement: .automatic) {
            Menu {
                Button("Export as PDF…") { exportPatchbay(asPDF: true) }
                Button("Export as Image (PNG)…") { exportPatchbay(asPDF: false) }
            } label: {
                Label("Export Patchbay", systemImage: "square.and.arrow.up")
            }
            .disabled(state.parsedDevices == nil)
            .help("Export the full Patchbay as a PDF or image — useful to print and keep near the rack")
        }

        ToolbarItem(placement: .automatic) {
            Button(action: { openOrFocusSettingsWindow { openWindow(id: "physicalDeviceSettings") } }) {
                Label("Converters & Interfaces", systemImage: "slider.horizontal.3")
            }
            .help("Manage converters, audio interfaces, and physical channel mapping")
        }
    }

    private func exportPatchbay(asPDF: Bool) {
        guard let devices = state.parsedDevices else { return }
        if asPDF {
            PatchbayExporter.exportPDF(devices: devices, controlRoomBuses: state.controlRoomBuses, cellScale: cellScale)
        } else {
            PatchbayExporter.exportPNG(devices: devices, controlRoomBuses: state.controlRoomBuses, cellScale: cellScale)
        }
    }

    private func statBadge(value: Int, label: String, color: Color) -> some View {
        VStack(spacing: 0) {
            Text("\(value)")
                .font(.system(.headline, design: .monospaced))
                .foregroundStyle(color)
            Text(label)
                .font(.caption2)
                .foregroundStyle(.tertiary)
        }
    }
}

// MARK: - Filter Mode

enum FilterMode: String, CaseIterable, Identifiable {
    case all      = "All"
    case inst     = "Instruments"
    case fx       = "External FX"
    case sendOnly = "Send Only"
    case stereo   = "Stereo"
    case mono     = "Mono"
    case midi     = "Has MIDI"

    var id: String { rawValue }
    var icon: String {
        switch self {
        case .all:      return "square.grid.2x2"
        case .inst:     return "pianokeys"
        case .fx:       return "slider.horizontal.3"
        case .sendOnly: return "arrow.up.right"
        case .stereo:   return "speaker.wave.2"
        case .mono:     return "speaker.wave.1"
        case .midi:     return "cable.connector"
        }
    }
}
