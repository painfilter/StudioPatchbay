#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace StudioPatchbay::Export
{

enum class Format { png, pdf };

// What a view hands over for export: its whole content (not just the part
// scrolled into view), the colour behind it, and the zoom it is laid out at.
struct ExportSource
{
    juce::Component& content;
    juce::Colour background;
    double uiScale = 1.0;
    juce::String title;
};

// Renders the content at twice its logical (100 % zoom) size, capped so a very
// large patchbay cannot exhaust memory.
juce::Image renderForExport (const ExportSource& source);

// Writes PNG or PDF atomically: a temporary file next to the target, renamed
// over it only when complete.
juce::Result exportTo (const ExportSource& source, Format format, const juce::File& target);

} // namespace StudioPatchbay::Export
