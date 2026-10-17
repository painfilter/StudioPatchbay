#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace StudioPatchbay::DesktopIcons
{

// StudioPatchbay's own desktop icons, drawn in code with hard one-pixel edges
// and the Platinum bevel. Each paints into the tile's icon area (32 x 27
// logical) in either palette. No system artwork is used.
void volume (juce::Graphics&, juce::Rectangle<int>, bool dark);       // a Cubase installation
void converter (juce::Graphics&, juce::Rectangle<int>, bool dark);    // Converters & Interfaces
void printer (juce::Graphics&, juce::Rectangle<int>, bool dark);      // PDF export
void camera (juce::Graphics&, juce::Rectangle<int>, bool dark);       // PNG export
void watcher (juce::Graphics&, juce::Rectangle<int>, bool dark, bool watching);
void document (juce::Graphics&, juce::Rectangle<int>, bool dark, const juce::String& kind);
void trash (juce::Graphics&, juce::Rectangle<int>, bool dark);

} // namespace StudioPatchbay::DesktopIcons
