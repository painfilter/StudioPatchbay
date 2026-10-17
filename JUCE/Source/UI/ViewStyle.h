#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "Fonts.h"

namespace StudioPatchbay
{

// How the content views draw: the palette, the window zoom and the content
// zoom (60–200 %, the Swift app's "cell scale") on top of it.
struct ViewStyle
{
    Design::Palette pal = Design::lightPalette();
    bool dark = false;
    double uiScale = 1.0;
    double contentScale = 1.0;

    // Content measures scale with both zooms; chrome only with the window's.
    float k() const { return static_cast<float> (uiScale * contentScale); }
    int px (float logical) const { return juce::roundToInt (logical * k()); }
    juce::Font font (float logical, Fonts::Weight w = Fonts::Weight::regular) const { return Fonts::sans (logical * k(), w); }
    juce::Font mono (float logical) const;

    // The device-type colours of the Swift app (blue, orange, purple) and the
    // Control Room teal, adjusted to read on the Platinum surfaces.
    juce::Colour typeColour (ParsedDevice::Type) const;
    juce::Colour controlRoom() const;
    juce::Colour warning() const;
    juce::Colour midi() const;
    // Text in a type colour on a light or dark surface.
    juce::Colour onSurface (juce::Colour accent) const { return dark ? accent.brighter (0.35f) : accent.darker (0.35f); }

    // The colour of a channel cell for the devices on it (several: instrument
    // wins unless an effect is there too, as in the Swift app).
    juce::Colour cellColour (const std::vector<ParsedDevice>&) const;
};

} // namespace StudioPatchbay
