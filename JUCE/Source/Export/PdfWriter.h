#pragma once

#include <juce_core/juce_core.h>

namespace StudioPatchbay::Export
{

// A single-page PDF holding one RGB raster image that fills the page.
//
// Deliberately minimal: JUCE has no PDF backend, and a raster page is what a
// patchbay printout needs - a sheet to keep next to the rack. Writing the file
// format here (PDF 1.4, one Flate-compressed image XObject) keeps the export
// identical on macOS and Windows without a platform printing API.
struct PdfImagePage
{
    int pixelWidth = 0;
    int pixelHeight = 0;
    const juce::uint8* rgb = nullptr;   // pixelWidth * pixelHeight * 3 bytes, rows top to bottom
    int pageWidthPoints = 0;            // 1 point = 1/72 inch
    int pageHeightPoints = 0;
    juce::String title;
};

// Returns the complete PDF file, or an empty block if the page is invalid.
juce::MemoryBlock writeImagePdf (const PdfImagePage& page);

} // namespace StudioPatchbay::Export
