#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace StudioPatchbay
{

// The bundled UI typeface for the whole product family: IBM Plex Sans (SIL Open
// Font License 1.1, embedded from Resources/Fonts via BinaryData). Bundling
// means pixel-identical rendering on macOS and Windows regardless of installed
// system fonts, and an owned look instead of the generic platform fallback.
//
// The Typefaces live in a DeletedAtShutdown singleton so they never outlive JUCE.
struct Fonts
{
    enum class Weight { regular, medium, semiBold, bold };

    static juce::Typeface::Ptr typeface (Weight = Weight::regular);

    // A Font at the given height in the chosen weight.
    static juce::Font sans (float height, Weight = Weight::regular);
    static juce::Font sans (float height, juce::Font::FontStyleFlags);

    // Route the chrome's fonts and a LookAndFeel's default sans-serif family
    // through Plex, so everything the look and feel draws uses the bundled face.
    static void applyTo (juce::LookAndFeel&);
};

} // namespace StudioPatchbay
