// swift-tools-version: 5.9
import PackageDescription

// The app itself is built from StudioPatchbay.xcodeproj. This package exists so
// the parsing layer can be tested and run in CI without building a GUI app.
//
// It deliberately compiles ONLY the pure-Foundation parsers, the part that reads
// a file format Steinberg controls and can change without warning. Everything
// SwiftUI-shaped stays out.
let package = Package(
    name: "StudioPatchbayCore",
    platforms: [.macOS(.v13)],
    targets: [
        .target(
            name: "PatchbayCore",
            path: "StudioPatchbay/Sources",
            exclude: [
                "Info.plist",
                "Assets.xcassets",
                "StudioPatchbayApp.swift",
                "ContentView.swift",
                "DesignSystem.swift",
                "Views.swift",
                "PhysicalDeviceSettingsView.swift",
                "PatchbayExporter.swift",
                "CubaseDetector.swift",
            ],
            sources: [
                "ExternalPluginsParser.swift",
                "PortSetupParser.swift",
                "ControlRoomParser.swift",
            ]
        ),
        .testTarget(
            name: "PatchbayCoreTests",
            dependencies: ["PatchbayCore"],
            path: "Tests/PatchbayCoreTests"
        ),
    ]
)
