#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace StudioPatchbay::Glyphs
{

// Symbols the bundled Plex fonts do not carry, drawn as paths so no system
// font ever stands in for them.
enum class Kind { warning, stop, check };

void draw (juce::Graphics&, Kind, juce::Rectangle<float> area, juce::Colour);

// A symbol at the start of a line of text, both in the current font's size.
// The text uses the font already set on the Graphics.
void line (juce::Graphics&, Kind, juce::Colour symbolColour, juce::Colour textColour, const juce::String& text,
           juce::Rectangle<int> area, int maximumLines = 1);

// A filled triangle pointing left or right (Back / Forward).
void triangle (juce::Graphics&, juce::Rectangle<float> area, bool pointsRight, juce::Colour);

} // namespace StudioPatchbay::Glyphs
