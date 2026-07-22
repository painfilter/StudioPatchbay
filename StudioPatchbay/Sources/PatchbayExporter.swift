import SwiftUI
import AppKit
import UniformTypeIdentifiers

/// Exports the Patchbay as a static PDF or PNG — useful to print and keep near the
/// rack, or hand to someone else troubleshooting the studio.
///
/// Renders `PatchbayView.exportableContent` directly (not `body`), which is the same
/// content without the ScrollView wrapper — a ScrollView clips to its viewport, so
/// rendering `body` would only capture whatever happened to be visible on screen at
/// export time, not the full patchbay. `exportableContent` lays out at its natural
/// full height instead.
@MainActor
enum PatchbayExporter {
    private static func snapshotView(
        devices: ParsedDevices, controlRoomBuses: [ControlRoomBus], cellScale: Double
    ) -> some View {
        PatchbayView(
            devices: devices,
            highlightedDevices: [],       // no active search in a static snapshot
            cellScale: cellScale,
            isSearchActive: false,
            controlRoomBuses: controlRoomBuses,
            selectedDevice: .constant(nil),
            showDetail: .constant(false),
            scrollRequest: .constant(nil),
            showConnectedDevices: true    // export-only: list connected Cubase devices under each block
        )
        .exportableContent
        // A fixed width gives the layout system something definite to lay out
        // against; height is left to size naturally to the content, the same way
        // an actual window would compute it.
        .frame(width: 1400)
        .background(Color(nsColor: .windowBackgroundColor))
    }

    static func exportPDF(devices: ParsedDevices, controlRoomBuses: [ControlRoomBus], cellScale: Double) {
        let renderer = ImageRenderer(
            content: snapshotView(devices: devices, controlRoomBuses: controlRoomBuses, cellScale: cellScale)
        )

        let panel = NSSavePanel()
        panel.allowedContentTypes = [.pdf]
        panel.nameFieldStringValue = "StudioPatchbay.pdf"
        panel.canCreateDirectories = true
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            renderer.render(rasterizationScale: 2.0) { size, context in
                var mediaBox = CGRect(origin: .zero, size: size)
                guard let consumer = CGDataConsumer(url: url as CFURL),
                      let pdfContext = CGContext(consumer: consumer, mediaBox: &mediaBox, nil) else { return }
                pdfContext.beginPDFPage(nil)
                context(pdfContext)
                pdfContext.endPDFPage()
                pdfContext.closePDF()
            }
        }
    }

    static func exportPNG(devices: ParsedDevices, controlRoomBuses: [ControlRoomBus], cellScale: Double) {
        let renderer = ImageRenderer(
            content: snapshotView(devices: devices, controlRoomBuses: controlRoomBuses, cellScale: cellScale)
        )
        renderer.scale = 2.0   // retina-quality raster output

        let panel = NSSavePanel()
        panel.allowedContentTypes = [.png]
        panel.nameFieldStringValue = "StudioPatchbay.png"
        panel.canCreateDirectories = true
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            guard let nsImage = renderer.nsImage,
                  let tiff = nsImage.tiffRepresentation,
                  let bitmap = NSBitmapImageRep(data: tiff),
                  let data = bitmap.representation(using: .png, properties: [:])
            else { return }
            try? data.write(to: url)
        }
    }
}
