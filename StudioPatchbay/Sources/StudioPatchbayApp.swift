import SwiftUI
import AppKit

/// Explicitly checks for an already-open settings window before creating one.
/// `openWindow(id:)` alone was assumed to dedupe automatically for a plain
/// WindowGroup(id:) with no associated data value — in practice that didn't hold up,
/// and multiple settings windows could be opened. This checks AppKit directly via the
/// window's title (set via .navigationTitle in PhysicalDeviceSettingsView) and brings
/// an existing one to the front instead of ever creating a second.
@MainActor
func openOrFocusSettingsWindow(fallback: () -> Void) {
    if let existing = NSApplication.shared.windows.first(where: { $0.title == "StudioPatchbay Settings" }) {
        existing.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
    } else {
        fallback()
    }
}

@main
struct StudioPatchbayApp: App {
    @StateObject private var state = AppState()
    @Environment(\.openWindow) private var openWindow

    var body: some Scene {
        WindowGroup {
            ContentView(state: state)
                .frame(minWidth: 900, minHeight: 600)
        }
        .windowStyle(.titleBar)
        .windowToolbarStyle(.unified)
        .commands {
            CommandGroup(replacing: .newItem) { }
        }

        // A WindowGroup, not a Settings scene: the converter/interface manager is a
        // full management window (lists, forms, import/export), not a lightweight
        // preferences pane — and Settings scenes are specifically modeled on the
        // classic fixed-size macOS Preferences convention, which kept re-asserting
        // itself no matter how aggressively we forced resizability via AppKit.
        // WindowGroup is resizable by default, no workarounds needed. Singleton
        // behavior (not opening a second window) is enforced explicitly via
        // openOrFocusSettingsWindow, rather than relied upon as automatic.
        WindowGroup(id: "physicalDeviceSettings") {
            PhysicalDeviceSettingsView(state: state)
        }
        .commands {
            CommandGroup(replacing: .appSettings) {
                Button("Settings…") {
                    openOrFocusSettingsWindow { openWindow(id: "physicalDeviceSettings") }
                }
                .keyboardShortcut(",", modifiers: .command)
            }
        }
    }
}
