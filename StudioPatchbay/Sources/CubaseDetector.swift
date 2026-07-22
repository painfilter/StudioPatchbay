import Foundation
import Combine

/// Represents one detected Cubase installation and its External Plugins XML path.
struct CubaseInstallation: Identifiable, Hashable {
    let version: Int          // e.g. 15 or 16
    let prefsURL: URL         // …/Preferences/Cubase 15/
    var xmlURL: URL { prefsURL.appendingPathComponent("External Plugins.xml") }
    var controlRoomXMLURL: URL {
        prefsURL.appendingPathComponent("Presets/ControlRoom/Default.xml")
    }
    var portSetupXMLURL: URL { prefsURL.appendingPathComponent("Port Setup.xml") }
    var id: URL { prefsURL }
    var displayName: String   { "Cubase \(version)" }
    var xmlExists: Bool       { FileManager.default.fileExists(atPath: xmlURL.path) }
    var controlRoomXMLExists: Bool { FileManager.default.fileExists(atPath: controlRoomXMLURL.path) }
    var portSetupXMLExists: Bool { FileManager.default.fileExists(atPath: portSetupXMLURL.path) }
}

/// Scans ~/Library/Preferences for all "Cubase NN" folders (any version ≥ 12).
/// Returns them sorted newest-first so the app defaults to the latest version.
func detectCubaseInstallations() -> [CubaseInstallation] {
    let fm = FileManager.default
    guard let prefsDir = fm.urls(for: .libraryDirectory, in: .userDomainMask).first?
            .appendingPathComponent("Preferences") else { return [] }

    guard let entries = try? fm.contentsOfDirectory(
        at: prefsDir,
        includingPropertiesForKeys: [.isDirectoryKey],
        options: .skipsHiddenFiles
    ) else { return [] }

    return entries
        .compactMap { url -> CubaseInstallation? in
            let name = url.lastPathComponent
            guard name.hasPrefix("Cubase ") else { return nil }
            let versionStr = name.dropFirst("Cubase ".count)
            guard let version = Int(versionStr), version >= 12 else { return nil }
            var isDir: ObjCBool = false
            guard fm.fileExists(atPath: url.path, isDirectory: &isDir), isDir.boolValue else { return nil }
            return CubaseInstallation(version: version, prefsURL: url)
        }
        .sorted { $0.version > $1.version }
}

// MARK: - File System Watcher

/// Watches a single file path using DispatchSource (kqueue-based, low overhead).
/// Fires `onChange` on the main queue whenever the file is written.
final class FileWatcher {
    private var source: DispatchSourceFileSystemObject?
    private var fd: Int32 = -1
    var onChange: (() -> Void)?

    func start(watching url: URL) {
        stop()
        fd = open(url.path, O_EVTONLY)
        guard fd >= 0 else { return }

        source = DispatchSource.makeFileSystemObjectSource(
            fileDescriptor: fd,
            eventMask: [.write, .rename, .delete],
            queue: .main
        )
        source?.setEventHandler { [weak self] in
            // Cubase writes atomically (rename), so re-open after rename
            self?.handleEvent(url: url)
        }
        source?.setCancelHandler { [weak self] in
            if let fd = self?.fd, fd >= 0 { close(fd) }
        }
        source?.resume()
    }

    private func handleEvent(url: URL) {
        // Small delay to let Cubase finish the atomic write before we read
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) { [weak self] in
            self?.onChange?()
            // If the file was replaced (rename), restart watching the new inode
            self?.restart(url: url)
        }
    }

    private func restart(url: URL) {
        stop()
        // Only restart if file still exists (Cubase might not have recreated it yet)
        if FileManager.default.fileExists(atPath: url.path) {
            start(watching: url)
        } else {
            // Poll briefly until file reappears (Cubase can take a moment)
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [weak self] in
                self?.restart(url: url)
            }
        }
    }

    func stop() {
        source?.cancel()
        source = nil
        if fd >= 0 { close(fd); fd = -1 }
    }

    deinit { stop() }
}

// MARK: - App State

@MainActor
final class AppState: ObservableObject {
    @Published var installations: [CubaseInstallation] = []
    @Published var selectedInstallation: CubaseInstallation?
    @Published var parsedDevices: ParsedDevices?
    @Published var controlRoomBuses: [ControlRoomBus] = []
    @Published var detectedPorts: [DetectedPort] = []
    @Published var physicalDevices: [PhysicalDevice] = PhysicalDeviceLayout.defaultSeed
    @Published var physicalDeviceSaveError: String?
    @Published var portLabelOverrides: [String: String] = [:]
    @Published var isWatching: Bool = false
    @Published var lastReloadTime: Date?
    @Published var errorMessage: String?

    private let watcher = FileWatcher()
    private let controlRoomWatcher = FileWatcher()
    private let portSetupWatcher = FileWatcher()

    /// Where physicalDevices.json actually lives — surfaced in Settings via "Reveal
    /// Save File in Finder…" and now via physicalDeviceSaveError if creating the
    /// folder fails, instead of silently doing nothing (a real bug: a swallowed
    /// error here meant a failed save looked identical to a successful one, with
    /// zero feedback).
    /// Tries Application Support first, then falls back to other writable
    /// locations if that's blocked — discovered the hard way: a user's
    /// ~/Library/Application Support was owned by root (likely from a system-
    /// patching tool), which silently prevented every app on that machine from
    /// creating a new subfolder there, not just this one. Rather than fail
    /// permanently in that situation, degrade to somewhere that does work.
    private static func resolveWritableAppDataDir() -> URL {
        let fm = FileManager.default
        let candidates: [URL] = [
            fm.urls(for: .applicationSupportDirectory, in: .userDomainMask).first,
            fm.urls(for: .documentDirectory, in: .userDomainMask).first,
            fm.urls(for: .cachesDirectory, in: .userDomainMask).first
        ].compactMap { $0 }

        for base in candidates {
            let dir = base.appendingPathComponent("StudioPatchbay", isDirectory: true)
            do {
                try fm.createDirectory(at: dir, withIntermediateDirectories: true)
                return dir
            } catch {
                print("StudioPatchbay: couldn't use \(dir.path): \(error)")
            }
        }
        // Last resort — always writable, though the OS may periodically clear it.
        let tempDir = fm.temporaryDirectory.appendingPathComponent("StudioPatchbay", isDirectory: true)
        try? fm.createDirectory(at: tempDir, withIntermediateDirectories: true)
        return tempDir
    }

    // Renamed from the legacy "converters.json" to match the PhysicalDevice rename
    // throughout the rest of the codebase — safe to do now specifically because
    // there's only one real user of this app so far, doing the on-disk rename
    // manually themselves. Not something to repeat casually once there's a wider
    // user base with existing data riding on the old name.
    let physicalDevicesFileURL: URL = {
        let fm = FileManager.default
        let dir = resolveWritableAppDataDir()
        let fileURL = dir.appendingPathComponent("physicalDevices.json")

        // Self-healing: an earlier build apparently left an actual DIRECTORY sitting
        // at this exact path instead of a file — the real root cause of the earlier
        // persistence bug. Every save since has been silently failing to write a
        // file into a path already occupied by a folder of the same name. Clear it
        // out so a normal file can live here again.
        var isDir: ObjCBool = false
        if fm.fileExists(atPath: fileURL.path, isDirectory: &isDir), isDir.boolValue {
            try? fm.removeItem(at: fileURL)
        }

        return fileURL
    }()

    /// User-supplied physical-interface labels for auto-detected ports (e.g. typing
    /// "RME HDSPe MADI FX" for the generic "MADI Interface" section). Same directory
    /// as physicalDevices.json, just a separate file — this is fundamentally different
    /// data (a label per auto-detected port name, not a user-defined device).
    let portLabelsFileURL: URL = {
        let dir = resolveWritableAppDataDir()
        return dir.appendingPathComponent("portLabels.json")
    }()

    init() {
        loadPhysicalDevices()
        loadPortLabels()
        refresh()
    }

    /// Re-scans for installations and reloads XML from the selected one.
    private static let lastVersionKey = "StudioPatchbay.lastSelectedCubaseVersion"

    func refresh() {
        installations = detectCubaseInstallations()

        if selectedInstallation == nil || !installations.contains(where: { $0 == selectedInstallation }) {
            // Prefer whatever version was selected last time (if it's still present
            // and has real data), rather than always defaulting to "first with XML
            // found" — a person who's set up multiple Cubase versions shouldn't have
            // to reselect their preferred one every single launch.
            let lastVersion = UserDefaults.standard.integer(forKey: Self.lastVersionKey)
            let remembered = installations.first { $0.version == lastVersion && $0.xmlExists }
            selectedInstallation = remembered ?? installations.first(where: \.xmlExists) ?? installations.first
        }

        reloadPortSetupXML()
        reloadXML()
        reloadControlRoomXML()
    }

    func select(_ installation: CubaseInstallation) {
        selectedInstallation = installation
        UserDefaults.standard.set(installation.version, forKey: Self.lastVersionKey)
        reloadPortSetupXML()
        reloadXML()
        reloadControlRoomXML()
        startWatching()
    }

    func reloadXML() {
        guard let inst = selectedInstallation else { parsedDevices = nil; return }
        guard inst.xmlExists else {
            errorMessage = "External Plugins.xml not found in \(inst.prefsURL.path)"
            parsedDevices = nil
            return
        }

        do {
            let data = try Data(contentsOf: inst.xmlURL)
            parsedDevices = try ExternalPluginsParser.parse(data: data)
            errorMessage = nil
            lastReloadTime = Date()
        } catch {
            errorMessage = "Failed to parse XML: \(error.localizedDescription)"
        }
    }

    /// Control Room is optional — not every setup uses it, so a missing file just
    /// means an empty overlay, not an error like a missing External Plugins.xml would be.
    func reloadControlRoomXML() {
        guard let inst = selectedInstallation, inst.controlRoomXMLExists else {
            controlRoomBuses = []
            return
        }
        do {
            let data = try Data(contentsOf: inst.controlRoomXMLURL)
            controlRoomBuses = try ControlRoomParser.parse(data: data)
        } catch {
            // Don't clobber errorMessage (reserved for the primary XML) — Control
            // Room parsing failures just fall back to no overlay.
            controlRoomBuses = []
        }
    }

    /// Rebuilds the entire physical port layout from Port Setup.xml. Like Control
    /// Room, a missing/unparseable file doesn't clobber errorMessage — AudioPortLayout
    /// just keeps whatever it last had rather than going blank.
    func reloadPortSetupXML() {
        guard let inst = selectedInstallation, inst.portSetupXMLExists else { return }
        do {
            let data = try Data(contentsOf: inst.portSetupXMLURL)
            let detected = try PortSetupParser.parse(data: data)
            detectedPorts = detected
            AudioPortLayout.updatePorts(from: detected)
            AudioPortLayout.applyLabelOverrides(portLabelOverrides)
        } catch {
            // Keep previous good AudioPortLayout state on failure.
        }
    }

    // MARK: - Port labels (user-supplied physical-interface names)

    func loadPortLabels() {
        guard let data = try? Data(contentsOf: portLabelsFileURL),
              let decoded = try? JSONDecoder().decode([String: String].self, from: data) else {
            return
        }
        portLabelOverrides = decoded
    }

    private func savePortLabels() {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        guard let data = try? encoder.encode(portLabelOverrides) else { return }
        try? data.write(to: portLabelsFileURL, options: .atomic)
    }

    /// Sets (or clears, if `label` is blank) the physical-interface label for a given
    /// auto-detected port name, re-applying it immediately so the Patchbay reflects it
    /// without needing a reload.
    func setPortLabel(_ label: String, for portName: String) {
        let trimmed = label.trimmingCharacters(in: .whitespaces)
        if trimmed.isEmpty {
            portLabelOverrides.removeValue(forKey: portName)
        } else {
            portLabelOverrides[portName] = trimmed
        }
        AudioPortLayout.applyLabelOverrides(portLabelOverrides)
        savePortLabels()
    }

    func startWatching() {
        guard let inst = selectedInstallation, inst.xmlExists else { return }
        watcher.onChange = { [weak self] in
            self?.reloadXML()
        }
        watcher.start(watching: inst.xmlURL)
        isWatching = true

        controlRoomWatcher.stop()
        if inst.controlRoomXMLExists {
            controlRoomWatcher.onChange = { [weak self] in
                self?.reloadControlRoomXML()
            }
            controlRoomWatcher.start(watching: inst.controlRoomXMLURL)
        }

        portSetupWatcher.stop()
        if inst.portSetupXMLExists {
            portSetupWatcher.onChange = { [weak self] in
                self?.reloadPortSetupXML()
            }
            portSetupWatcher.start(watching: inst.portSetupXMLURL)
        }
    }

    func stopWatching() {
        watcher.stop()
        controlRoomWatcher.stop()
        portSetupWatcher.stop()
        isWatching = false
    }

    // MARK: - Physical device settings (Milestone 2)

    func loadPhysicalDevices() {
        let fm = FileManager.default
        guard fm.fileExists(atPath: physicalDevicesFileURL.path) else {
            // Genuinely first launch — nothing to load yet, not an error.
            physicalDevices = PhysicalDeviceLayout.defaultSeed
            savePhysicalDevices()
            return
        }
        do {
            let data = try Data(contentsOf: physicalDevicesFileURL)
            let decoded = try JSONDecoder().decode([PhysicalDevice].self, from: data)
            physicalDevices = decoded
            PhysicalDeviceLayout.devices = physicalDevices
            physicalDeviceSaveError = nil
        } catch {
            // The file exists but couldn't be read/decoded — this should NOT
            // silently reset to defaults and lose whatever's actually on disk from
            // view. Surface it, and leave `physicalDevices` as whatever it already
            // was (the in-memory default) rather than guessing.
            physicalDeviceSaveError = "Couldn't read \(physicalDevicesFileURL.path):\n\(error.localizedDescription)"
        }
    }

    private func savePhysicalDevices() {
        PhysicalDeviceLayout.devices = physicalDevices
        let fm = FileManager.default
        var isDir: ObjCBool = false
        if fm.fileExists(atPath: physicalDevicesFileURL.path, isDirectory: &isDir), isDir.boolValue {
            try? fm.removeItem(at: physicalDevicesFileURL)
        }
        do {
            let encoder = JSONEncoder()
            encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
            let data = try encoder.encode(physicalDevices)
            try data.write(to: physicalDevicesFileURL, options: .atomic)
            physicalDeviceSaveError = nil
        } catch {
            // This used to be `try?` on both steps — completely silent on failure,
            // meaning a failed save looked identical to a successful one. That's
            // the actual bug: devices would show up in the UI (driven by the
            // in-memory array, unaffected by disk state) with zero indication that
            // nothing was actually persisted.
            physicalDeviceSaveError = "Couldn't save to \(physicalDevicesFileURL.path):\n\(error.localizedDescription)"
        }
    }

    func addPhysicalDevice(_ device: PhysicalDevice) {
        physicalDevices.append(device)
        savePhysicalDevices()
    }

    func updatePhysicalDevice(_ device: PhysicalDevice) {
        guard let idx = physicalDevices.firstIndex(where: { $0.id == device.id }) else { return }
        physicalDevices[idx] = device
        savePhysicalDevices()
    }

    func deletePhysicalDevice(_ device: PhysicalDevice) {
        physicalDevices.removeAll { $0.id == device.id }
        savePhysicalDevices()
    }

    /// Import — Replace All: wholesale swap, used when loading a backup/template
    /// that should become the entire device list.
    func replaceAllPhysicalDevices(with newDevices: [PhysicalDevice]) {
        physicalDevices = newDevices
        savePhysicalDevices()
    }

    /// Duplicate: copies an existing converter/interface with a fresh UUID, so
    /// managing a set of similar devices (e.g. several identical Ferrofish units)
    /// only requires changing the name and channel numbers afterward rather than
    /// re-entering everything from scratch.
    func duplicatePhysicalDevice(_ device: PhysicalDevice) -> PhysicalDevice {
        let copy = PhysicalDevice(
            name: "\(device.name) copy", shortName: device.shortName,
            portColor: device.portColor, deviceType: device.deviceType,
            physicalChannelCount: device.physicalChannelCount,
            sendChannelStart: device.sendChannelStart, recvChannelStart: device.recvChannelStart,
            directionality: device.directionality, customColorHex: device.customColorHex
        )
        physicalDevices.append(copy)
        savePhysicalDevices()
        return copy
    }

    /// Import — Add to Current: appends imported devices with brand-new UUIDs.
    /// Their original IDs (from whoever exported them, at some unrelated time) get
    /// discarded entirely rather than renumbered — with UUIDs there's no "next
    /// available slot" to compute, and no realistic chance of a fresh one colliding
    /// with anything already in this list.
    func addPhysicalDevices(_ imported: [PhysicalDevice]) {
        let fresh = imported.map { d in
            PhysicalDevice(
                name: d.name, shortName: d.shortName, portColor: d.portColor,
                deviceType: d.deviceType, physicalChannelCount: d.physicalChannelCount,
                sendChannelStart: d.sendChannelStart, recvChannelStart: d.recvChannelStart,
                directionality: d.directionality, customColorHex: d.customColorHex
            )
        }
        physicalDevices.append(contentsOf: fresh)
        savePhysicalDevices()
    }

    /// Checks a candidate device against every other device for direction-
    /// specific channel overlap — the exact class of bug (overlapping send/recv
    /// ranges between neighboring devices) that caused real display issues earlier
    /// in this app's development. Returns a human-readable conflict per collision;
    /// empty means the candidate is safe to save.
    func physicalDeviceConflicts(for candidate: PhysicalDevice) -> [String] {
        var issues: [String] = []
        for direction in [ChannelDirection.send, .recv] where candidate.supports(direction) {
            let candidateRange = candidate.range(for: direction)
            for other in physicalDevices where other.id != candidate.id && other.supports(direction) {
                let otherRange = other.range(for: direction)
                guard candidateRange.overlaps(otherRange) else { continue }
                let dirLabel = direction == .send ? "OUT" : "IN"
                let lo = max(candidateRange.lowerBound, otherRange.lowerBound)
                let hi = min(candidateRange.upperBound, otherRange.upperBound)
                issues.append("\(dirLabel) ch \(lo)–\(hi) overlaps \"\(other.name)\"")
            }
        }
        return issues
    }
}
