import SwiftUI
import AppKit
import UniformTypeIdentifiers

// MARK: - Physical Device Settings (Milestone 2)

struct PhysicalDeviceSettingsView: View {
    @ObservedObject var state: AppState
    @Environment(\.dismiss) private var dismiss

    @State private var editingDevice: PhysicalDevice?
    @State private var isPresentingNew = false
    @State private var pendingDelete: PhysicalDevice?
    @State private var pendingRemoveAll = false
    @State private var pendingImportReview: ImportReview?
    @State private var importError: String?
    // nil = "All" (grouped by section); a specific value filters to just that type.
    @State private var deviceTypeFilter: DeviceType?

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()
            HSplitView {
                deviceList
                    .frame(minWidth: 340)
                detectedPortsReference
                    .frame(minWidth: 260)
            }
        }
        .navigationTitle("StudioPatchbay Settings")
        .frame(minWidth: 760, idealWidth: 980, maxWidth: 1200,
               minHeight: 520, idealHeight: 760, maxHeight: 920)
        .sheet(item: $editingDevice) { device in
            PhysicalDeviceEditView(state: state, device: device, isNew: false)
        }
        .sheet(isPresented: $isPresentingNew) {
            PhysicalDeviceEditView(
                state: state,
                device: PhysicalDevice(
                    name: "New Converter",
                    shortName: "New",
                    portColor: .other,
                    physicalChannelCount: 8,
                    sendChannelStart: 1,
                    recvChannelStart: 1
                ),
                isNew: true
            )
        }
        .alert("Delete this converter/interface?", isPresented: Binding(
            get: { pendingDelete != nil },
            set: { if !$0 { pendingDelete = nil } }
        )) {
            Button("Cancel", role: .cancel) { pendingDelete = nil }
            Button("Delete", role: .destructive) {
                if let d = pendingDelete { state.deletePhysicalDevice(d) }
                pendingDelete = nil
            }
        } message: {
            Text("This only removes it from the app's display — it doesn't change anything in Cubase.")
        }
        .alert("Remove all converters and interfaces?", isPresented: $pendingRemoveAll) {
            Button("Cancel", role: .cancel) { }
            Button("Remove All", role: .destructive) { state.replaceAllPhysicalDevices(with: []) }
        } message: {
            Text("This clears your entire list. Consider exporting a backup first — this can't be undone.")
        }
        .sheet(item: $pendingImportReview) { review in
            ImportReviewView(
                devices: review.devices,
                existingDevices: state.physicalDevices,
                detectedPorts: state.detectedPorts,
                onAdd: { state.addPhysicalDevices(review.devices) },
                onReplace: { state.replaceAllPhysicalDevices(with: review.devices) }
            )
        }
        .alert("Import Failed", isPresented: Binding(
            get: { importError != nil },
            set: { if !$0 { importError = nil } }
        )) {
            Button("OK") { importError = nil }
        } message: {
            Text(importError ?? "")
        }
        .alert("Converter Save Problem", isPresented: Binding(
            get: { state.physicalDeviceSaveError != nil },
            set: { if !$0 { state.physicalDeviceSaveError = nil } }
        )) {
            Button("OK") { state.physicalDeviceSaveError = nil }
        } message: {
            Text(state.physicalDeviceSaveError ?? "")
        }
    }

    private var header: some View {
        HStack(spacing: 12) {
            Text("Converters & Interfaces")
                .font(.title3.weight(.semibold))
            Spacer()
            Button {
                isPresentingNew = true
            } label: {
                Label("Add", systemImage: "plus")
            }
            Menu {
                Button("Export…") { exportDevices() }
                Button("Import…") { importDevices() }
                Divider()
                Button("Reveal Save File in Finder…") {
                    NSWorkspace.shared.activateFileViewerSelecting([state.physicalDevicesFileURL])
                }
                Button("Copy Save File Path") {
                    NSPasteboard.general.clearContents()
                    NSPasteboard.general.setString(state.physicalDevicesFileURL.path, forType: .string)
                }
                Divider()
                Button("Remove All", role: .destructive) {
                    pendingRemoveAll = true
                }
            } label: {
                Image(systemName: "ellipsis.circle")
            }
            .menuStyle(.borderlessButton)
            .frame(width: 28)
            Button("Close") { dismiss() }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
    }

    // MARK: - Export / Import

    private func exportDevices() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.json]
        panel.nameFieldStringValue = "StudioPatchbay Physical Devices.json"
        panel.canCreateDirectories = true
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            do {
                let encoder = JSONEncoder()
                encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
                let data = try encoder.encode(state.physicalDevices)
                try data.write(to: url)
            } catch {
                importError = "Couldn't save the file: \(error.localizedDescription)"
            }
        }
    }

    private func importDevices() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.json]
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            do {
                let data = try Data(contentsOf: url)
                let decoded = try JSONDecoder().decode([PhysicalDevice].self, from: data)
                guard !decoded.isEmpty else {
                    importError = "That file doesn't contain any converters or interfaces."
                    return
                }
                pendingImportReview = ImportReview(devices: decoded)
            } catch {
                importError = "Couldn't read that file as a converter/interface export: \(error.localizedDescription)"
            }
        }
    }

    private var deviceList: some View {
        Group {
            if state.physicalDevices.isEmpty {
                ContentUnavailableView {
                    Label("Nothing Configured Yet", systemImage: "cable.connector.slash")
                } description: {
                    Text("Add the converters and interfaces in your studio, or import a setup someone else has shared with you.")
                } actions: {
                    Button("Add") { isPresentingNew = true }
                    Button("Import…") { importDevices() }
                }
            } else {
                VStack(spacing: 0) {
                    deviceTypeFilterBar
                    Divider()
                    if deviceTypeFilter == nil {
                        List {
                            groupedSection(for: .converter, label: "Converters")
                            groupedSection(for: .audioInterface, label: "Audio Interfaces")
                            groupedSection(for: .other, label: "Other")
                        }
                    } else {
                        List(filteredDevices) { device in
                            deviceRow(device)
                        }
                    }
                }
            }
        }
    }

    // nil = "All" (grouped by section below); a specific value filters to just that type.
    private var deviceTypeFilterBar: some View {
        Picker("Filter", selection: $deviceTypeFilter) {
            Text("All").tag(DeviceType?.none)
            ForEach(DeviceType.allCases, id: \.self) { type in
                Text(type.displayName).tag(DeviceType?.some(type))
            }
        }
        .pickerStyle(.segmented)
        .labelsHidden()
        .padding(10)
    }

    private func devices(ofType type: DeviceType) -> [PhysicalDevice] {
        state.physicalDevices
            .filter { $0.deviceType == type }
            .sorted { $0.sendChannelStart < $1.sendChannelStart }
    }

    private var filteredDevices: [PhysicalDevice] {
        guard let type = deviceTypeFilter else { return [] }
        return devices(ofType: type)
    }

    @ViewBuilder
    private func groupedSection(for type: DeviceType, label: String) -> some View {
        let items = devices(ofType: type)
        if !items.isEmpty {
            Section(label) {
                ForEach(items) { device in
                    deviceRow(device)
                }
            }
        }
    }

    private func deviceRow(_ device: PhysicalDevice) -> some View {
        PhysicalDeviceRow(device: device)
            .contentShape(Rectangle())
            .onTapGesture { editingDevice = device }
            .swipeActions {
                Button("Delete", role: .destructive) { pendingDelete = device }
                Button("Duplicate") { editingDevice = state.duplicatePhysicalDevice(device) }
                    .tint(.blue)
            }
            .contextMenu {
                Button("Edit") { editingDevice = device }
                Button("Duplicate") { editingDevice = state.duplicatePhysicalDevice(device) }
                Divider()
                Button("Delete", role: .destructive) { pendingDelete = device }
            }
    }

    // Easy to overlook that this data is version-specific when you first open
    // Settings — shows which Cubase installation it came from, right where the
    // ports are displayed, with the ability to switch without leaving this window.
    private var cubaseVersionRow: some View {
        HStack(spacing: 6) {
            Image(systemName: "app.badge.checkmark").font(.caption2).foregroundStyle(.secondary)
            Text("From:").font(.caption).foregroundStyle(.secondary)
            if state.installations.isEmpty {
                Text("No installation detected").font(.caption.weight(.medium)).foregroundStyle(.orange)
            } else {
                Menu {
                    ForEach(state.installations) { inst in
                        Button {
                            state.select(inst)
                        } label: {
                            if state.selectedInstallation == inst {
                                Label(inst.displayName, systemImage: "checkmark")
                            } else {
                                Text(inst.displayName)
                            }
                        }
                    }
                } label: {
                    Text(state.selectedInstallation?.displayName ?? "Select…")
                        .font(.caption.weight(.medium))
                }
                .menuStyle(.borderlessButton)
                .fixedSize()
            }
            Spacer()
        }
    }

    private var detectedPortsReference: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Detected Physical Ports")
                .font(.callout.weight(.semibold))
                .padding(.horizontal, 12).padding(.top, 12).padding(.bottom, 4)
            Text("Auto-detected from Port Setup.xml — for reference while entering channel numbers below.")
                .font(.caption).foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
                .padding(.horizontal, 12).padding(.bottom, 6)
            cubaseVersionRow
                .padding(.horizontal, 12).padding(.bottom, 8)
            if !AudioPortLayout.ports.isEmpty {
                Divider()
                portLabelEditor
            }
            Divider()
            if state.detectedPorts.isEmpty {
                ContentUnavailableView("No ports detected", systemImage: "cable.connector",
                    description: Text("Open a Cubase project with this interface active, then refresh."))
                    .padding(.top, 20)
            } else {
                if hasApproximatePorts {
                    Label("* Approximate — this interface uses plain CoreAudio rather than ASIO, so Cubase's Port Setup.xml has no real channel numbers for it. Numbering below is a best guess (sequential per device), not confirmed Cubase data.",
                          systemImage: "exclamationmark.triangle.fill")
                        .font(.caption2).foregroundStyle(.orange)
                        .fixedSize(horizontal: false, vertical: true)
                        .padding(.horizontal, 12).padding(.vertical, 6)
                    Divider()
                }
                List(groupedDetectedPorts, id: \.self) { line in
                    Text(line).font(.system(.caption, design: .monospaced))
                }
            }
            Spacer(minLength: 0)
        }
    }

    private var hasApproximatePorts: Bool {
        state.detectedPorts.contains { $0.isApproximate }
    }
    // Optional, user-supplied — since we found (the hard way, by actually checking)
    // that Cubase's own data doesn't reliably tell us which physical card a MADI/ADAT
    // section belongs to when multiple interfaces are aggregated together. A person
    // who owns the hardware knows this with certainty; guessing from ambiguous file
    // structure risked being confidently wrong, which is worse than generic.
    private var portLabelEditor: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text("Physical Interface Names").font(.caption.weight(.semibold)).foregroundStyle(.secondary)
            Text("Optional — label each section with the actual hardware it's on (e.g. \"RME HDSPe MADI FX\"). Useful if you have more than one interface using the same protocol.")
                .font(.caption2).foregroundStyle(.tertiary)
                .fixedSize(horizontal: false, vertical: true)
            ForEach(AudioPortLayout.ports) { port in
                PortLabelRow(port: port, state: state)
            }
        }
        .padding(.horizontal, 12).padding(.vertical, 8)
    }

    private var groupedDetectedPorts: [String] {
        state.detectedPorts
            .sorted { $0.channelRange.lowerBound < $1.channelRange.lowerBound }
            .map { p in
                let dir = p.direction == .send ? "OUT" : "IN "
                let mark = p.isApproximate ? "*" : " "
                return "\(dir)\(mark) \(p.family.padding(toLength: 14, withPad: " ", startingAt: 0))ch \(p.channelRange.lowerBound)–\(p.channelRange.upperBound)"
            }
    }
}

private struct PhysicalDeviceRow: View {
    let device: PhysicalDevice

    private var swatch: Color {
        if let hex = device.customColorHex, let c = Color(hex: hex) { return c }
        return device.portColor.color
    }

    private var rangeText: String {
        switch device.directionality {
        case .bidirectional:
            return "OUT ch \(device.range(for: .send).lowerBound)–\(device.range(for: .send).upperBound)  ·  IN ch \(device.range(for: .recv).lowerBound)–\(device.range(for: .recv).upperBound)  ·  \(device.physicalChannelCount) ch"
        case .sendOnly:
            return "OUT ch \(device.range(for: .send).lowerBound)–\(device.range(for: .send).upperBound)  ·  \(device.physicalChannelCount) ch  ·  output only"
        case .recvOnly:
            return "IN ch \(device.range(for: .recv).lowerBound)–\(device.range(for: .recv).upperBound)  ·  \(device.physicalChannelCount) ch  ·  input only"
        }
    }

    var body: some View {
        HStack(spacing: 10) {
            RoundedRectangle(cornerRadius: 3).fill(swatch).frame(width: 10, height: 28)
            Image(systemName: device.deviceType.icon)
                .foregroundStyle(.secondary)
                .frame(width: 16)
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: 6) {
                    Text(device.name).font(.system(size: 13, weight: .semibold))
                    if device.deviceType != .converter {
                        Text(device.deviceType.displayName)
                            .font(.system(size: 10, weight: .medium))
                            .padding(.horizontal, 5).padding(.vertical, 1)
                            .background(.quaternary, in: Capsule())
                            .foregroundStyle(.secondary)
                    }
                }
                Text(rangeText)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundStyle(.secondary)
            }
            Spacer()
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Add/Edit Form

private struct PhysicalDeviceEditView: View {
    @ObservedObject var state: AppState
    let isNew: Bool
    @Environment(\.dismiss) private var dismiss

    private let original: PhysicalDevice
    @State private var draft: PhysicalDevice
    @State private var useCustomColor: Bool
    @State private var customColor: Color
    @State private var showDiscardConfirm = false

    init(state: AppState, device: PhysicalDevice, isNew: Bool) {
        self.state = state
        self.isNew = isNew
        self.original = device
        _draft = State(initialValue: device)
        if let hex = device.customColorHex, let c = Color(hex: hex) {
            _useCustomColor = State(initialValue: true)
            _customColor = State(initialValue: c)
        } else {
            _useCustomColor = State(initialValue: false)
            _customColor = State(initialValue: device.portColor.color)
        }
    }

    private var conflicts: [String] { state.physicalDeviceConflicts(for: draft) }
    private var canSave: Bool {
        !draft.name.trimmingCharacters(in: .whitespaces).isEmpty && conflicts.isEmpty && draft.physicalChannelCount > 0
    }

    // What would actually be saved right now, including the color picker state that
    // only gets folded into `draft` inside save(). Comparing this against the
    // original is what lets Cancel/Escape/click-away warn before silently discarding
    // real work — this is the fix for the exact class of bug that likely caused
    // earlier data loss (closing the form without an explicit Save).
    private var pendingDevice: PhysicalDevice {
        var d = draft
        d.customColorHex = useCustomColor ? customColor.toHex() : nil
        return d
    }
    private var hasUnsavedChanges: Bool { pendingDevice != original }

    var body: some View {
        NavigationStack {
            Form {
                Section {
                    TextField("Name", text: $draft.name)
                    TextField("Short Name (for compact labels)", text: $draft.shortName)
                    Picker("Device Type", selection: $draft.deviceType) {
                        ForEach(DeviceType.allCases, id: \.self) { type in
                            Label(type.displayName, systemImage: type.icon).tag(type)
                        }
                    }
                } header: {
                    Text("Identity")
                } footer: {
                    Text("Converter: a box like a Ferrofish or ADI-648 that only sits between the RME's digital port and external gear — it has no connection to the computer itself. Audio Interface: a self-contained USB/Thunderbolt device (RME, Universal Audio, MOTU, etc.) with its own analog I/O.")
                }

                Section {
                    Picker("Connector Type", selection: $draft.portColor) {
                        ForEach(ConnectorType.digitalCases, id: \.self) { type in
                            Label {
                                Text(type.displayName)
                            } icon: {
                                Circle().fill(type.color).frame(width: 8, height: 8)
                            }
                            .tag(type)
                        }
                    }
                    Toggle("Use a custom color instead", isOn: $useCustomColor)
                    if useCustomColor {
                        ColorPicker("Custom Color", selection: $customColor, supportsOpacity: false)
                    }
                } header: {
                    Text("Connector Type & Color")
                } footer: {
                    Text("The digital protocol this entry uses to connect to the audio interface — not what's on its other side (which might well be analog/mic jacks). This is just categorization and coloring; it has no effect on the channel numbers below.")
                }

                Section {
                    Picker("Directions", selection: $draft.directionality) {
                        ForEach(DeviceDirectionality.allCases, id: \.self) { d in
                            Text(d.displayName).tag(d)
                        }
                    }
                } header: {
                    Text("Directionality")
                } footer: {
                    Text("Most have both directions. Pick Output/Input only for hardware that's genuinely one-way — a monitor system with no input path, a mic preamp with no line output, etc. If a physical connector's OUT and IN sides go to two unrelated pieces of gear, model the one that actually goes through conversion here, and leave the other side to show with no converter — it's a direct digital connection, not something needing an AD/DA conversion.")
                }

                Section {
                    intField("Physical I/O count", value: $draft.physicalChannelCount, range: 1...128)
                    if draft.supports(.send) {
                        Picker("OUT (send) starts at channel", selection: $draft.sendChannelStart) {
                            ForEach(startChannelOptions(for: .send), id: \.self) { ch in
                                Text("\(ch)").tag(ch)
                            }
                        }
                        LabeledContent("Resulting OUT range") {
                            Text("\(draft.range(for: .send).lowerBound)–\(draft.range(for: .send).upperBound)")
                                .font(.system(.body, design: .monospaced)).foregroundStyle(.secondary)
                        }
                    }
                    if draft.supports(.recv) {
                        Picker("IN (recv) starts at channel", selection: $draft.recvChannelStart) {
                            ForEach(startChannelOptions(for: .recv), id: \.self) { ch in
                                Text("\(ch)").tag(ch)
                            }
                        }
                        LabeledContent("Resulting IN range") {
                            Text("\(draft.range(for: .recv).lowerBound)–\(draft.range(for: .recv).upperBound)")
                                .font(.system(.body, design: .monospaced)).foregroundStyle(.secondary)
                        }
                    }
                } header: {
                    Text("Channels")
                } footer: {
                    Text("OUT and IN usually start at the same channel. They only differ when Cubase itself numbers this connector's output and input sides differently in its aggregate channel list — a numbering quirk of the audio interface, not something about the connector. Check Port Setup.xml (reference panel) if you're not sure. Only start positions that don't overlap another converter/interface are listed — this entry's own current channels are always included.")
                }

                if !conflicts.isEmpty {
                    Section {
                        ForEach(conflicts, id: \.self) { issue in
                            Label(issue, systemImage: "exclamationmark.triangle.fill")
                                .foregroundStyle(.orange)
                                .font(.caption)
                        }
                    } header: {
                        Text("Channel Conflicts")
                    } footer: {
                        Text("These channels are already claimed by another converter/interface for the same direction. Adjust the ranges above before saving.")
                    }
                }
            }
            .formStyle(.grouped)
            .navigationTitle(isNew ? "Add Converter / Interface" : "Edit Converter / Interface")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        if hasUnsavedChanges { showDiscardConfirm = true } else { dismiss() }
                    }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Save") { save() }.disabled(!canSave)
                }
            }
        }
        .frame(minWidth: 520, minHeight: 620)
        .interactiveDismissDisabled(hasUnsavedChanges)
        .alert("Discard changes?", isPresented: $showDiscardConfirm) {
            Button("Keep Editing", role: .cancel) { }
            Button("Discard", role: .destructive) { dismiss() }
        } message: {
            Text("You'll lose the changes you made here.")
        }
    }

    private func save() {
        draft.customColorHex = useCustomColor ? customColor.toHex() : nil

        if isNew {
            state.addPhysicalDevice(draft)
        } else {
            state.updatePhysicalDevice(draft)
        }
        dismiss()
    }

    // Text field for direct typing (fast for multi-digit numbers) plus a small
    // stepper alongside for one-click fine adjustment — clicking a stepper repeatedly
    // to reach a two-digit channel number was tedious.
    private func intField(_ label: String, value: Binding<Int>, range: ClosedRange<Int>) -> some View {
        HStack {
            Text(label)
            Spacer()
            TextField("", value: value, formatter: NumberFormatter())
                .multilineTextAlignment(.trailing)
                .frame(width: 64)
                .textFieldStyle(.roundedBorder)
            Stepper("", value: value, in: range)
                .labelsHidden()
        }
    }

    /// Every start-channel value for which a `physicalChannelCount`-wide range,
    /// beginning there, wouldn't overlap any OTHER device's range for the same
    /// direction. This device's own current channels are deliberately excluded
    /// from the "occupied" set, so editing an existing device never locks out its
    /// own already-assigned range.
    private func startChannelOptions(for direction: ChannelDirection) -> [Int] {
        var occupied = Set<Int>()
        for other in state.physicalDevices where other.id != draft.id && other.supports(direction) {
            occupied.formUnion(other.range(for: direction))
        }
        let count = max(draft.physicalChannelCount, 1)
        let ceiling = max((occupied.max() ?? 0) + count + 32, 256)

        var valid: [Int] = []
        for start in 1...ceiling {
            if !(start...(start + count - 1)).contains(where: occupied.contains) {
                valid.append(start)
            }
        }

        // Always include the currently selected value, even if it's no longer valid
        // (e.g. the physical count just changed) — otherwise the Picker would show a
        // blank selection instead of letting the Channel Conflicts section explain it.
        let current = direction == .send ? draft.sendChannelStart : draft.recvChannelStart
        if !valid.contains(current) { valid.append(current) }
        return valid.sorted()
    }
}

// MARK: - Import Review

private struct ImportReview: Identifiable {
    let id = UUID()
    let devices: [PhysicalDevice]
}

/// Shown after picking a file to import, before anything is actually committed.
///
/// Two independent checks happen here:
/// - Hardware compatibility (against `detectedPorts`) — informational only, since
///   it's legitimate to prep a config for hardware that isn't connected right now.
/// - Channel conflicts (against both the currently-saved devices AND other
///   devices within the same import batch) — this is NOT informational. The
///   single-item Add/Edit form already refuses to save an overlapping range;
///   Import must enforce the identical rule, or it becomes a backdoor around that
///   protection. That gap is exactly what caused a real overlapping-channel bug
///   here previously.
private struct ImportReviewView: View {
    let devices: [PhysicalDevice]
    let existingDevices: [PhysicalDevice]
    let detectedPorts: [DetectedPort]
    let onAdd: () -> Void
    let onReplace: () -> Void
    @Environment(\.dismiss) private var dismiss

    private func portIssues(for dev: PhysicalDevice) -> [String] {
        var result: [String] = []
        for direction in [ChannelDirection.send, .recv] where dev.supports(direction) {
            let range = dev.range(for: direction)
            let matches = detectedPorts.contains { $0.direction == direction && $0.channelRange.overlaps(range) }
            if !matches {
                let dirLabel = direction == .send ? "OUT" : "IN"
                result.append("\(dirLabel) ch \(range.lowerBound)–\(range.upperBound) — no matching port detected on this Mac")
            }
        }
        return result
    }

    // Generic overlap check against a comparison set, with NO self-exclusion — used
    // for checking against existingDevices, where every entry is genuinely
    // separate from anything in the import batch. There's no legitimate "self" to
    // exclude here, unlike the single-item edit form's conflict check.
    private func channelConflicts(for dev: PhysicalDevice, against others: [PhysicalDevice]) -> [String] {
        var issues: [String] = []
        for direction in [ChannelDirection.send, .recv] where dev.supports(direction) {
            let candidateRange = dev.range(for: direction)
            for other in others where other.supports(direction) {
                let otherRange = other.range(for: direction)
                guard candidateRange.overlaps(otherRange) else { continue }
                let dirLabel = direction == .send ? "OUT" : "IN"
                let lo = max(candidateRange.lowerBound, otherRange.lowerBound)
                let hi = min(candidateRange.upperBound, otherRange.upperBound)
                issues.append("\(dirLabel) ch \(lo)–\(hi) conflicts with \"\(other.name)\"")
            }
        }
        return issues
    }

    // Batch-internal conflicts use array-INDEX for self-exclusion, not id — imported
    // items can coincidentally share id values with each other or with existing
    // devices (IDs get renumbered independently on each import), so comparing by
    // id equality can wrongly treat two genuinely different devices as "the same
    // one, skip it." That exact bug let a real duplicate slip through undetected.
    private func internalBatchConflicts(atIndex i: Int) -> [String] {
        let dev = devices[i]
        var issues: [String] = []
        for direction in [ChannelDirection.send, .recv] where dev.supports(direction) {
            let candidateRange = dev.range(for: direction)
            for (j, other) in devices.enumerated() where j != i && other.supports(direction) {
                let otherRange = other.range(for: direction)
                guard candidateRange.overlaps(otherRange) else { continue }
                let dirLabel = direction == .send ? "OUT" : "IN"
                let lo = max(candidateRange.lowerBound, otherRange.lowerBound)
                let hi = min(candidateRange.upperBound, otherRange.upperBound)
                issues.append("\(dirLabel) ch \(lo)–\(hi) conflicts with \"\(other.name)\"")
            }
        }
        return issues
    }

    /// Conflicts relevant if importing via "Add to Current": against what's already
    /// saved, plus against the rest of this same batch.
    private func addConflicts(atIndex i: Int) -> [String] {
        channelConflicts(for: devices[i], against: existingDevices) + internalBatchConflicts(atIndex: i)
    }
    /// Conflicts relevant if importing via "Replace All": existing devices get
    /// wiped first, so only internal batch conflicts matter.
    private func replaceConflicts(atIndex i: Int) -> [String] {
        internalBatchConflicts(atIndex: i)
    }

    private var mismatchCount: Int {
        devices.filter { !portIssues(for: $0).isEmpty }.count
    }
    private var addConflictCount: Int {
        devices.indices.filter { !addConflicts(atIndex: $0).isEmpty }.count
    }
    private var replaceConflictCount: Int {
        devices.indices.filter { !replaceConflicts(atIndex: $0).isEmpty }.count
    }

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Review Import").font(.title3.weight(.semibold))
                Spacer()
                Button("Cancel") { dismiss() }
            }
            .padding()
            Divider()

            VStack(alignment: .leading, spacing: 6) {
                if detectedPorts.isEmpty {
                    Label("No ports currently detected on this Mac — open a Cubase project using your interface, then refresh, for a meaningful compatibility check. Proceeding without one just skips the check.",
                          systemImage: "info.circle")
                        .font(.caption).foregroundStyle(.secondary)
                } else if mismatchCount > 0 {
                    Label("\(mismatchCount) of \(devices.count) don't match any port detected on this Mac — likely made for different hardware.",
                          systemImage: "exclamationmark.triangle.fill")
                        .font(.caption).foregroundStyle(.orange)
                } else {
                    Label("All \(devices.count) match ports detected on this Mac.", systemImage: "checkmark.circle.fill")
                        .font(.caption).foregroundStyle(.green)
                }

                if addConflictCount > 0 {
                    Label("\(addConflictCount) have a channel conflict with your existing devices (or each other) — \"Add to Current\" is disabled until resolved.",
                          systemImage: "xmark.octagon.fill")
                        .font(.caption).foregroundStyle(.red)
                }
                if replaceConflictCount > 0 {
                    Label("\(replaceConflictCount) conflict with each other within this import itself — \"Replace All\" is disabled until resolved.",
                          systemImage: "xmark.octagon.fill")
                        .font(.caption).foregroundStyle(.red)
                }
            }
            .fixedSize(horizontal: false, vertical: true)
            .padding()

            Divider()

            List(Array(devices.enumerated()), id: \.offset) { i, dev in
                VStack(alignment: .leading, spacing: 3) {
                    Text(dev.name).font(.system(size: 13, weight: .semibold))
                    let pIssues = portIssues(for: dev)
                    let cIssues = addConflicts(atIndex: i)
                    if pIssues.isEmpty && cIssues.isEmpty {
                        Label("No issues", systemImage: "checkmark.circle.fill")
                            .font(.caption2).foregroundStyle(.green)
                    } else {
                        ForEach(cIssues, id: \.self) { issue in
                            Label(issue, systemImage: "xmark.octagon.fill")
                                .font(.caption2).foregroundStyle(.red)
                        }
                        ForEach(pIssues, id: \.self) { issue in
                            Label(issue, systemImage: "exclamationmark.triangle.fill")
                                .font(.caption2).foregroundStyle(.orange)
                        }
                    }
                }
                .padding(.vertical, 2)
            }

            Divider()
            HStack {
                Spacer()
                Button("Add to Current") { onAdd(); dismiss() }
                    .disabled(addConflictCount > 0)
                Button("Replace All", role: .destructive) { onReplace(); dismiss() }
                    .disabled(replaceConflictCount > 0)
            }
            .padding()
        }
        .frame(minWidth: 520, minHeight: 480)
    }
}

// MARK: - Port Label Row

/// A single editable row in "Physical Interface Names". Typing itself only touches
/// local @State — instant, no lag. The actual commit (state.setPortLabel, which
/// writes to disk, rebuilds AudioPortLayout.ports, and triggers the main window's
/// full-refresh mechanism) only fires on Return or on losing focus, not per
/// keystroke — binding the TextField straight to AppState meant every single
/// character typed was forcing the entire Patchbay to rebuild from scratch.
private struct PortLabelRow: View {
    let port: AudioPort
    @ObservedObject var state: AppState
    @State private var text: String = ""
    @FocusState private var isFocused: Bool

    private var stableKey: String { AudioPortLayout.stableLabelKey(for: port) }
    // Three-tier fallback matching AudioPortLayout.applyLabelOverrides — a label
    // saved under any of the three key schemes this app has used should still show
    // up here until it's next edited and rewritten under the current key.
    private var currentSavedValue: String {
        state.portLabelOverrides[stableKey]
            ?? state.portLabelOverrides["ch\(port.channelRange.lowerBound)"]
            ?? state.portLabelOverrides[port.name]
            ?? ""
    }

    var body: some View {
        HStack(spacing: 8) {
            Text(port.name)
                .font(.caption.monospaced())
                .frame(width: 60, alignment: .leading)
            TextField("\(port.name) Interface", text: $text)
                .textFieldStyle(.roundedBorder)
                .font(.caption)
                .focused($isFocused)
                .onSubmit { commit() }
        }
        .onAppear { text = currentSavedValue }
        .onChange(of: isFocused) { _, focused in
            if !focused { commit() }
        }
    }

    private func commit() {
        guard text != currentSavedValue else { return }
        state.setPortLabel(text, for: stableKey)
    }
}
